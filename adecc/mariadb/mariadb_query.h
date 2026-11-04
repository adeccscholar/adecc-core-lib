// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
#pragma once
/** \file mariadb_query.h \brief Typed MariaDB Connector/C query backend. */
#include "mariadb_database.h"
#include "mariadb_sql.h"
#include <mysql.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <format>
#include <string>
#include <variant>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <expected>
#include <limits>
#include <memory>
#include <utility>
#include <optional>
#include <ranges>
#include <tuple>
#include <vector>

namespace adecc::db::mariadb {
template<adecc::db::framework_database_type DB>
class query {
   struct stmt_cleanup {void operator()(MYSQL_STMT* p)const noexcept{if(p)mysql_stmt_close(p);}};
   struct meta_cleanup {void operator()(MYSQL_RES* p)const noexcept{if(p)mysql_free_result(p);}};
   DB* db_{};
   std::unique_ptr<MYSQL_STMT,stmt_cleanup> stmt_;
   std::unique_ptr<MYSQL_RES,meta_cleanup> meta_;
   detail::parameter_plan plan_,outputPlan_;
   adecc::db_output_parameters output_;
   std::optional<std::size_t> identity_;
   std::vector<std::string> columns_;
   // Native connector storage. Fixed-size columns are fetched directly into
   // their C++ representation; only variable-size columns need a second fetch.
   struct native_cell {
      enum enum_field_types eSqlType{MYSQL_TYPE_NULL};
      bool boUnsigned{};
      bool boVariable{};
      bool boBinary{};
      bool boBitOne{};
      std::int8_t iTiny{};
      std::uint8_t uTiny{};
      std::int16_t iShort{};
      std::uint16_t uShort{};
      std::int32_t iLong{};
      std::uint32_t uLong{};
      std::int64_t iSigned{};
      std::uint64_t uUnsigned{};
      float flFloat{};
      double flValue{};
      MYSQL_TIME aTime{};
      std::string strValue{};
      std::vector<std::byte> vecBinary{};
   };
   std::vector<native_cell> row_;
   std::vector<MYSQL_BIND> resultBinds_;
   std::unique_ptr<unsigned long[]> resultLengths_;
   std::unique_ptr<my_bool[]> resultNulls_,resultErrors_;

   [[nodiscard]] static adecc::date_ty NativeDate(MYSQL_TIME const& aTime) {
      return adecc::date_ty{
         std::chrono::year{static_cast<int>(aTime.year)},
         std::chrono::month{aTime.month},
         std::chrono::day{aTime.day}
      };
   }
   [[nodiscard]] static adecc::timestamp_ty NativeTimestamp(MYSQL_TIME const& aTime) {
      auto const aDate = NativeDate(aTime);
      if(!aDate.ok()) throw std::runtime_error{"Invalid MariaDB date"};
      return std::chrono::sys_days{aDate}
         + std::chrono::hours{aTime.hour}
         + std::chrono::minutes{aTime.minute}
         + std::chrono::seconds{aTime.second}
         + std::chrono::microseconds{aTime.second_part};
   }
   [[nodiscard]] static adecc::time_ty NativeTime(MYSQL_TIME const& aTime) {
      auto const aDuration =
         std::chrono::hours{aTime.hour}
         + std::chrono::minutes{aTime.minute}
         + std::chrono::seconds{aTime.second};
      return adecc::time_ty{aTime.neg ? -aDuration : aDuration};
   }
   [[nodiscard]] static MYSQL_TIME BindDate(adecc::date_ty const& aDate) {
      if(!aDate.ok()) throw std::runtime_error{"Invalid MariaDB date parameter"};
      MYSQL_TIME aResult{};
      aResult.year=static_cast<unsigned int>(static_cast<int>(aDate.year()));
      aResult.month=static_cast<unsigned int>(aDate.month());
      aResult.day=static_cast<unsigned int>(aDate.day());
      aResult.time_type=MYSQL_TIMESTAMP_DATE;
      return aResult;
   }
   [[nodiscard]] static MYSQL_TIME BindTimestamp(adecc::timestamp_ty const& aTimestamp) {
      auto const aDay=std::chrono::floor<std::chrono::days>(aTimestamp);
      MYSQL_TIME aResult=BindDate(adecc::date_ty{aDay});
      auto const aSubday=std::chrono::duration_cast<std::chrono::microseconds>(
         aTimestamp-aDay);
      auto uTotal=static_cast<std::uint64_t>(aSubday.count());
      aResult.hour=static_cast<unsigned int>(uTotal/3600000000ULL);
      uTotal%=3600000000ULL;
      aResult.minute=static_cast<unsigned int>(uTotal/60000000ULL);
      uTotal%=60000000ULL;
      aResult.second=static_cast<unsigned int>(uTotal/1000000ULL);
      aResult.second_part=static_cast<unsigned long>(uTotal%1000000ULL);
      aResult.time_type=MYSQL_TIMESTAMP_DATETIME;
      return aResult;
   }
   [[nodiscard]] static MYSQL_TIME BindTime(adecc::time_ty const& aTime) {
      MYSQL_TIME aResult{};
      auto const aDuration=aTime.to_duration();
      aResult.neg=aDuration < std::chrono::seconds::zero();
      auto const aSeconds=std::chrono::duration_cast<std::chrono::seconds>(
         aResult.neg ? -aDuration : aDuration).count();
      aResult.hour=static_cast<unsigned int>(aSeconds/3600);
      aResult.minute=static_cast<unsigned int>((aSeconds%3600)/60);
      aResult.second=static_cast<unsigned int>(aSeconds%60);
      aResult.time_type=MYSQL_TIMESTAMP_TIME;
      return aResult;
   }
   template<class T, class U>
   [[nodiscard]] static T ConvertNative(U const& aValue) {
      if constexpr(std::same_as<T,U>) return aValue;
      else if constexpr(std::integral<T> && std::integral<U> &&
                        !std::same_as<T,bool> && !std::same_as<U,bool>) {
         if(!std::in_range<T>(aValue))
            throw std::out_of_range{"MariaDB integer out of target range"};
         return static_cast<T>(aValue);
      }
      else if constexpr(std::same_as<T,adecc::money_ty> && std::is_arithmetic_v<U>) {
         return adecc::money_ty{static_cast<double>(aValue)};
      }
      else if constexpr(std::integral<T> && std::floating_point<U>) {
         if(!std::isfinite(aValue) || std::trunc(aValue)!=aValue ||
            static_cast<long double>(aValue) <
               static_cast<long double>(std::numeric_limits<T>::lowest()) ||
            static_cast<long double>(aValue) >
               static_cast<long double>(std::numeric_limits<T>::max()))
            throw std::out_of_range{"MariaDB floating-point value is not an exact target integer"};
         return static_cast<T>(aValue);
      }
      else if constexpr(requires { adecc::ConvertTo<T>(aValue); })
         return adecc::ConvertTo<T>(aValue);
      else throw std::runtime_error{"Unsupported MariaDB native type conversion"};
   }
   template<class T>
   [[nodiscard]] static T ConvertCell(native_cell const& aCell) {
      if(aCell.boBitOne) {
         if(aCell.vecBinary.size()!=1)
            throw std::runtime_error{"Invalid MariaDB BIT(1) length"};
         return ConvertNative<T>(aCell.vecBinary[0]!=std::byte{0});
      }
      if(aCell.boVariable) {
         if(aCell.boBinary) {
            if constexpr(std::same_as<T,adecc::Blob>) {
               return adecc::Blob{aCell.vecBinary};
            }
            else throw std::runtime_error{"Binary MariaDB value requires Blob"};
         }
         if constexpr(std::same_as<T,std::wstring>)
            return detail::Utf8ToWide(aCell.strValue);
         else return ConvertNative<T>(aCell.strValue);
      }
      switch(aCell.eSqlType) {
         case MYSQL_TYPE_TINY:
            return aCell.boUnsigned
               ? ConvertNative<T>(aCell.uTiny) : ConvertNative<T>(aCell.iTiny);
         case MYSQL_TYPE_SHORT:
         case MYSQL_TYPE_YEAR:
            return aCell.boUnsigned
               ? ConvertNative<T>(aCell.uShort) : ConvertNative<T>(aCell.iShort);
         case MYSQL_TYPE_LONG:
         case MYSQL_TYPE_INT24:
            return aCell.boUnsigned
               ? ConvertNative<T>(aCell.uLong) : ConvertNative<T>(aCell.iLong);
         case MYSQL_TYPE_LONGLONG:
            return aCell.boUnsigned
               ? ConvertNative<T>(aCell.uUnsigned) : ConvertNative<T>(aCell.iSigned);
         case MYSQL_TYPE_FLOAT:
            return ConvertNative<T>(aCell.flFloat);
         case MYSQL_TYPE_DOUBLE:
            return ConvertNative<T>(aCell.flValue);
         case MYSQL_TYPE_DATE:
         case MYSQL_TYPE_NEWDATE:
            return ConvertNative<T>(NativeDate(aCell.aTime));
         case MYSQL_TYPE_TIME:
            return ConvertNative<T>(NativeTime(aCell.aTime));
         case MYSQL_TYPE_DATETIME:
         case MYSQL_TYPE_TIMESTAMP:
            return ConvertNative<T>(NativeTimestamp(aCell.aTime));
         default: throw std::runtime_error{"Unsupported native MariaDB column"};
      }
   }
   bool executed_{},hasRow_{};

   [[nodiscard]] static error_ty Err(std::string m,std::string d) {
      return {{},std::move(m),std::move(d)};
   }
   void Reset() {
      meta_.reset();stmt_.reset();
      columns_.clear();row_.clear();resultBinds_.clear();
      resultLengths_.reset();resultNulls_.reset();resultErrors_.reset();
      executed_=false;hasRow_=false;
   }
   [[nodiscard]] framework_result_ty Run(detail::parameter_plan const& plan,
                                          adecc::db_params const& params) {
      Reset();
      try {
         if(!db_||!db_->Connected())
            return std::unexpected(Err("MariaDB","Not connected"));
         for(std::size_t i=0;i<params.size();++i) {
            for(std::size_t j=i+1;j<params.size();++j)
               if(detail::Equal(std::get<0>(params[i]),std::get<0>(params[j])))
                  return std::unexpected(Err("MariaDB","Duplicate parameter"));
            if(std::get<2>(params[i])&&!detail::Present(plan,std::get<0>(params[i])))
               return std::unexpected(Err("MariaDB","Required parameter missing in SQL"));
         }
         stmt_.reset(mysql_stmt_init(db_->NativeHandle()));
         if(!stmt_)return std::unexpected(Err("MariaDB","mysql_stmt_init failed"));
         if(mysql_stmt_prepare(stmt_.get(),plan.sql.data(),static_cast<unsigned long>(plan.sql.size())))
            return std::unexpected(Err("MariaDB prepare",mysql_stmt_error(stmt_.get())));
         if(mysql_stmt_param_count(stmt_.get())!=plan.names.size())
            return std::unexpected(Err("MariaDB","Parameter count mismatch"));
         struct param_cell {
            MYSQL_BIND aBind{};
            unsigned long uLength{};
            my_bool boNull{};
            std::int32_t iLong{};
            std::uint32_t uLong{};
            std::int64_t iSigned{};
            std::uint64_t uUnsigned{};
            double flValue{};
            signed char iBool{};
            MYSQL_TIME aTime{};
            std::string strValue{};
            adecc::Blob const* pBlob{};
         };
         // Keep every value and its MYSQL_BIND storage stable until execute.
         std::vector<param_cell> cells(plan.names.size());
         for(std::size_t i=0;i<plan.names.size();++i) {
            auto const& name=plan.names[i];
            auto item=std::ranges::find_if(params,[&](auto const& p){
               return detail::Equal(std::get<0>(p),name);
            });
            if(item==params.end())
               return std::unexpected(Err("MariaDB","Unbound parameter: "+name));
            auto& aCell=cells[i];
            std::visit([&]<class T>(T const& aValue) {
               using value_ty=std::remove_cvref_t<T>;
               if constexpr(adecc::is_optional_v<value_ty>) {
                  if(aValue) {
                     using native_ty=typename value_ty::value_type;
                     auto const& aInner=*aValue;
                     if constexpr(std::same_as<native_ty,adecc::Blob>) {
                        aCell.pBlob=&aInner;
                        aCell.aBind.buffer_type=MYSQL_TYPE_BLOB;
                     }
                     else if constexpr(std::same_as<native_ty,bool>) {
                        aCell.iBool=static_cast<signed char>(aInner);
                        aCell.aBind.buffer_type=MYSQL_TYPE_TINY;
                        aCell.aBind.buffer=&aCell.iBool;
                     }
                     else if constexpr(std::same_as<native_ty,int>) {
                        aCell.iLong=aInner;
                        aCell.aBind.buffer_type=MYSQL_TYPE_LONG;
                        aCell.aBind.buffer=&aCell.iLong;
                     }
                     else if constexpr(std::same_as<native_ty,long long>) {
                        aCell.iSigned=aInner;
                        aCell.aBind.buffer_type=MYSQL_TYPE_LONGLONG;
                        aCell.aBind.buffer=&aCell.iSigned;
                     }
                     else if constexpr(std::same_as<native_ty,unsigned int>) {
                        aCell.uLong=aInner;
                        aCell.aBind.buffer_type=MYSQL_TYPE_LONG;
                        aCell.aBind.is_unsigned=1;
                        aCell.aBind.buffer=&aCell.uLong;
                     }
                     else if constexpr(std::same_as<native_ty,unsigned long long>) {
                        aCell.uUnsigned=aInner;
                        aCell.aBind.buffer_type=MYSQL_TYPE_LONGLONG;
                        aCell.aBind.is_unsigned=1;
                        aCell.aBind.buffer=&aCell.uUnsigned;
                     }
                     else if constexpr(std::same_as<native_ty,double>) {
                        aCell.flValue=aInner;
                        aCell.aBind.buffer_type=MYSQL_TYPE_DOUBLE;
                        aCell.aBind.buffer=&aCell.flValue;
                     }
                     else if constexpr(std::same_as<native_ty,adecc::date_ty> ||
                                       std::same_as<native_ty,adecc::timestamp_ty> ||
                                       std::same_as<native_ty,adecc::time_ty>) {
                        if constexpr(std::same_as<native_ty,adecc::date_ty>) {
                           aCell.aTime=BindDate(aInner);
                           aCell.aBind.buffer_type=MYSQL_TYPE_DATE;
                        }
                        else if constexpr(std::same_as<native_ty,adecc::timestamp_ty>) {
                           aCell.aTime=BindTimestamp(aInner);
                           aCell.aBind.buffer_type=MYSQL_TYPE_DATETIME;
                        }
                        else {
                           aCell.aTime=BindTime(aInner);
                           aCell.aBind.buffer_type=MYSQL_TYPE_TIME;
                        }
                        aCell.aBind.buffer=&aCell.aTime;
                        aCell.aBind.buffer_length=sizeof(MYSQL_TIME);
                     }
                     else {
                        aCell.strValue=*detail::Encode(aInner);
                        aCell.aBind.buffer_type=MYSQL_TYPE_STRING;
                     }
                  }
                  else aCell.boNull=1;
               }
               else if constexpr(std::same_as<value_ty,adecc::Blob>) {
                  aCell.pBlob=&aValue;
                  aCell.aBind.buffer_type=MYSQL_TYPE_BLOB;
               }
               else if constexpr(std::same_as<value_ty,bool>) {
                  aCell.iBool=static_cast<signed char>(aValue);
                  aCell.aBind.buffer_type=MYSQL_TYPE_TINY;
                  aCell.aBind.buffer=&aCell.iBool;
               }
               else if constexpr(std::same_as<value_ty,int>) {
                  aCell.iLong=aValue;
                  aCell.aBind.buffer_type=MYSQL_TYPE_LONG;
                  aCell.aBind.buffer=&aCell.iLong;
               }
               else if constexpr(std::same_as<value_ty,long long>) {
                  aCell.iSigned=aValue;
                  aCell.aBind.buffer_type=MYSQL_TYPE_LONGLONG;
                  aCell.aBind.buffer=&aCell.iSigned;
               }
               else if constexpr(std::same_as<value_ty,unsigned int>) {
                  aCell.uLong=aValue;
                  aCell.aBind.buffer_type=MYSQL_TYPE_LONG;
                  aCell.aBind.is_unsigned=1;
                  aCell.aBind.buffer=&aCell.uLong;
               }
               else if constexpr(std::same_as<value_ty,unsigned long long>) {
                  aCell.uUnsigned=aValue;
                  aCell.aBind.buffer_type=MYSQL_TYPE_LONGLONG;
                  aCell.aBind.is_unsigned=1;
                  aCell.aBind.buffer=&aCell.uUnsigned;
               }
               else if constexpr(std::same_as<value_ty,double>) {
                  aCell.flValue=aValue;
                  aCell.aBind.buffer_type=MYSQL_TYPE_DOUBLE;
                  aCell.aBind.buffer=&aCell.flValue;
               }
               else if constexpr(std::same_as<value_ty,adecc::date_ty> ||
                                 std::same_as<value_ty,adecc::timestamp_ty> ||
                                 std::same_as<value_ty,adecc::time_ty>) {
                  if constexpr(std::same_as<value_ty,adecc::date_ty>) {
                     aCell.aTime=BindDate(aValue);
                     aCell.aBind.buffer_type=MYSQL_TYPE_DATE;
                  }
                  else if constexpr(std::same_as<value_ty,adecc::timestamp_ty>) {
                     aCell.aTime=BindTimestamp(aValue);
                     aCell.aBind.buffer_type=MYSQL_TYPE_DATETIME;
                  }
                  else {
                     aCell.aTime=BindTime(aValue);
                     aCell.aBind.buffer_type=MYSQL_TYPE_TIME;
                  }
                  aCell.aBind.buffer=&aCell.aTime;
                  aCell.aBind.buffer_length=sizeof(MYSQL_TIME);
               }
               else {
                  auto aText=detail::Encode(aValue);
                  if(aText) aCell.strValue=std::move(*aText);
                  else aCell.boNull=1;
                  aCell.aBind.buffer_type=MYSQL_TYPE_STRING;
               }
            },std::get<1>(*item));
            aCell.aBind.is_null=&aCell.boNull;
            aCell.aBind.length=&aCell.uLength;
            if(!aCell.boNull) {
               if(aCell.pBlob) {
                  auto const aView=aCell.pBlob->View();
                  if(aView.size()>std::numeric_limits<unsigned long>::max())
                     return std::unexpected(Err("MariaDB bind","BLOB exceeds Connector/C buffer length"));
                  aCell.uLength=static_cast<unsigned long>(aView.size());
                  aCell.aBind.buffer=const_cast<std::byte*>(aView.data());
                  aCell.aBind.buffer_length=aCell.uLength;
               }
               else if(aCell.aBind.buffer_type==MYSQL_TYPE_STRING) {
                  if(aCell.strValue.size()>std::numeric_limits<unsigned long>::max())
                     return std::unexpected(Err("MariaDB bind","Parameter exceeds Connector/C buffer length"));
                  aCell.uLength=static_cast<unsigned long>(aCell.strValue.size());
                  aCell.aBind.buffer=aCell.strValue.data();
                  aCell.aBind.buffer_length=aCell.uLength;
               }
            }
         }
         std::vector<MYSQL_BIND> binds;
         binds.reserve(cells.size());
         for(auto const& aCell:cells) binds.push_back(aCell.aBind);
         if(!binds.empty()&&mysql_stmt_bind_param(stmt_.get(),binds.data()))
            return std::unexpected(Err("MariaDB bind",mysql_stmt_error(stmt_.get())));
         if(mysql_stmt_execute(stmt_.get()))
            return std::unexpected(Err("MariaDB execute",mysql_stmt_error(stmt_.get())));
         meta_.reset(mysql_stmt_result_metadata(stmt_.get()));
         if(meta_) {
            unsigned int n=mysql_num_fields(meta_.get());
            MYSQL_FIELD* fields=mysql_fetch_fields(meta_.get());
            columns_.reserve(n);row_.resize(n);resultBinds_.resize(n);
            resultLengths_.reset(new unsigned long[n]{});
            resultNulls_.reset(new my_bool[n]{});
            resultErrors_.reset(new my_bool[n]{});
            for(unsigned int i=0;i<n;++i) {
               columns_.emplace_back(fields[i].name);
               auto& b=resultBinds_[i];
               auto& aCell=row_[i];
               aCell.eSqlType=fields[i].type;
               aCell.boUnsigned=(fields[i].flags & UNSIGNED_FLAG)!=0;
               aCell.boBinary=(fields[i].flags & BINARY_FLAG)!=0;
               aCell.boBitOne=(fields[i].type==MYSQL_TYPE_BIT && fields[i].length==1);
               b.is_null=&resultNulls_[i];
               b.length=&resultLengths_[i];
               b.error=&resultErrors_[i];
               switch(fields[i].type) {
                  case MYSQL_TYPE_TINY:
                     b.buffer_type=MYSQL_TYPE_TINY;
                     b.is_unsigned=aCell.boUnsigned;
                     b.buffer=aCell.boUnsigned
                        ? static_cast<void*>(&aCell.uTiny) : static_cast<void*>(&aCell.iTiny);
                     b.buffer_length=sizeof(std::int8_t);
                     break;
                  case MYSQL_TYPE_SHORT:
                  case MYSQL_TYPE_YEAR:
                     b.buffer_type=MYSQL_TYPE_SHORT;
                     b.is_unsigned=aCell.boUnsigned;
                     b.buffer=aCell.boUnsigned
                        ? static_cast<void*>(&aCell.uShort) : static_cast<void*>(&aCell.iShort);
                     b.buffer_length=sizeof(std::int16_t);
                     break;
                  case MYSQL_TYPE_LONG:
                  case MYSQL_TYPE_INT24:
                     b.buffer_type=MYSQL_TYPE_LONG;
                     b.is_unsigned=aCell.boUnsigned;
                     b.buffer=aCell.boUnsigned
                        ? static_cast<void*>(&aCell.uLong) : static_cast<void*>(&aCell.iLong);
                     b.buffer_length=sizeof(std::int32_t);
                     break;
                  case MYSQL_TYPE_LONGLONG:
                     b.buffer_type=MYSQL_TYPE_LONGLONG;
                     b.is_unsigned=aCell.boUnsigned;
                     b.buffer=aCell.boUnsigned
                        ? static_cast<void*>(&aCell.uUnsigned) : static_cast<void*>(&aCell.iSigned);
                     b.buffer_length=sizeof(std::int64_t);
                     break;
                  case MYSQL_TYPE_FLOAT:
                     b.buffer_type=MYSQL_TYPE_FLOAT;
                     b.buffer=&aCell.flFloat;
                     b.buffer_length=sizeof(float);
                     break;
                  case MYSQL_TYPE_DOUBLE:
                     b.buffer_type=MYSQL_TYPE_DOUBLE;
                     b.buffer=&aCell.flValue;
                     b.buffer_length=sizeof(double);
                     break;
                  case MYSQL_TYPE_DATE:
                  case MYSQL_TYPE_NEWDATE:
                  case MYSQL_TYPE_TIME:
                  case MYSQL_TYPE_DATETIME:
                  case MYSQL_TYPE_TIMESTAMP:
                     b.buffer_type=fields[i].type==MYSQL_TYPE_DATE || fields[i].type==MYSQL_TYPE_NEWDATE
                        ? MYSQL_TYPE_DATE : fields[i].type==MYSQL_TYPE_TIME
                        ? MYSQL_TYPE_TIME : MYSQL_TYPE_DATETIME;
                     b.buffer=&aCell.aTime;
                     b.buffer_length=sizeof(MYSQL_TIME);
                     break;
                  default:
                     // Variable-length text, decimal and binary values are
                     // fetched as their native bytes, never formatted scalars.
                     aCell.boVariable=true;
                     aCell.boBinary=aCell.boBitOne || fields[i].type==MYSQL_TYPE_BIT ||
                        fields[i].type==MYSQL_TYPE_BLOB ||
                        fields[i].type==MYSQL_TYPE_TINY_BLOB ||
                        fields[i].type==MYSQL_TYPE_MEDIUM_BLOB ||
                        fields[i].type==MYSQL_TYPE_LONG_BLOB ||
                        fields[i].type==MYSQL_TYPE_GEOMETRY ||
                        aCell.boBinary;
                     b.buffer_type=aCell.boBinary?MYSQL_TYPE_BLOB:MYSQL_TYPE_STRING;
                     break;
               }
            }
            if(mysql_stmt_bind_result(stmt_.get(),resultBinds_.data())||
               mysql_stmt_store_result(stmt_.get()))
               return std::unexpected(Err("MariaDB result",mysql_stmt_error(stmt_.get())));
         }
         executed_=true;
         return true;
      }
      catch(std::exception const& ex) {
         return std::unexpected(Err("MariaDB",ex.what()));
      }
   }
   [[nodiscard]] framework_result_ty ReadRow() {
      if(!executed_||!meta_)return false;
      int code=mysql_stmt_fetch(stmt_.get());
      if(code==MYSQL_NO_DATA){hasRow_=false;return false;}
      if(code!=0&&code!=MYSQL_DATA_TRUNCATED)
         return std::unexpected(Err("MariaDB fetch",mysql_stmt_error(stmt_.get())));
      for(std::size_t i=0;i<row_.size();++i) {
         auto& aCell=row_[i];
         if(resultNulls_[i]) {
            aCell.strValue.clear();
            aCell.vecBinary.clear();
            continue;
         }
         if(!aCell.boVariable) {
            if(resultErrors_[i])
               return std::unexpected(Err("MariaDB fetch","Truncated native scalar"));
            continue;
         }
         MYSQL_BIND b{};
         b.buffer_type=aCell.boBinary?MYSQL_TYPE_BLOB:MYSQL_TYPE_STRING;
         if(aCell.boBinary) {
            aCell.vecBinary.resize(resultLengths_[i]);
            b.buffer=aCell.vecBinary.data();
            b.buffer_length=static_cast<unsigned long>(aCell.vecBinary.size());
         }
         else {
            aCell.strValue.resize(resultLengths_[i]);
            b.buffer=aCell.strValue.data();
            b.buffer_length=static_cast<unsigned long>(aCell.strValue.size());
         }
         if(b.buffer_length==0) continue;
         unsigned long uActual{};
         b.length=&uActual;
         if(mysql_stmt_fetch_column(stmt_.get(),&b,static_cast<unsigned int>(i),0))
            return std::unexpected(Err("MariaDB fetch column",mysql_stmt_error(stmt_.get())));
         if(uActual>b.buffer_length)
            return std::unexpected(Err("MariaDB fetch column","Truncated variable-length value"));
         if(aCell.boBinary) aCell.vecBinary.resize(uActual);
         else aCell.strValue.resize(uActual);
      }
      hasRow_=true;return true;
   }
public:
   using fw_query_type=MYSQL_STMT*;
   explicit query(DB const& db):db_(&const_cast<DB&>(db)) {
      if(!db_->Connected())throw std::runtime_error{"MariaDB query on disconnected database"};
   }
   [[nodiscard]] framework_result_ty SetSql(std::string const& sql) {
      try{auto p=detail::Plan(sql);Reset();plan_=std::move(p);return true;}
      catch(std::exception const& e){return std::unexpected(Err("MariaDB SQL",e.what()));}
   }
   [[nodiscard]] framework_result_ty Open(std::string const& sql,adecc::db_params const& p) {
      if(auto r=SetSql(sql);!r)return r;return Open(p);
   }
   [[nodiscard]] framework_result_ty Open(adecc::db_params const& p){return Run(plan_,p);}
   [[nodiscard]] framework_result_ty First(){
      if(!executed_||!meta_)return false;
      mysql_stmt_data_seek(stmt_.get(),0);return ReadRow();
   }
   [[nodiscard]] framework_result_ty Fetch(){return hasRow_?ReadRow():framework_result_ty{false};}
   [[nodiscard]] framework_result_ty Eof() const{return !hasRow_;}
   [[nodiscard]] std::vector<std::string> GetAttributes() const{return columns_;}
   template<class T>
   [[nodiscard]] framework_get_ty<T> GetField(std::string const& name)const {
      if(!hasRow_)return std::unexpected(Err("MariaDB field","No current row"));
      for(std::size_t i=0;i<columns_.size();++i)
         if(detail::Equal(columns_[i],name)){
            if(resultNulls_[i])return std::optional<T>{};
            try {
               return std::optional<T>{ConvertCell<T>(row_[i])};
            }
            catch(std::exception const& e){return std::unexpected(Err("MariaDB field",e.what()));}
         }
      return std::unexpected(Err("MariaDB field","Unknown field "+name));
   }
   [[nodiscard]] framework_output_ty ExecuteCommand(std::string const& sql){
      if(auto r=SetSql(sql);!r)return std::unexpected(r.error());
      if(!plan_.names.empty())return std::unexpected(Err("MariaDB command","Unexpected parameters"));
      if(auto r=Open({});!r)return std::unexpected(r.error());
      auto n=mysql_stmt_affected_rows(stmt_.get());
      return framework_output_result{static_cast<std::int64_t>(n==static_cast<my_ulonglong>(-1)?0:n),std::nullopt};
   }
   [[nodiscard]] framework_result_ty PrepareOutput(std::string const& sql,
      adecc::db_output_parameters const& fields){
      try{
         auto p=detail::Plan(sql);
         std::optional<std::size_t> identity;
         for(std::size_t i=0;i<fields.size();++i){
            auto const& [name,role]=fields[i];
            if(role==adecc::db_output_param_role::identity){
               if(identity||detail::Present(p,name))
                  return std::unexpected(Err("MariaDB output","Invalid identity declaration"));
               identity=i;
            }
            else if(role!=adecc::db_output_param_role::may_be_missing&&!detail::Present(p,name))
               return std::unexpected(Err("MariaDB output","Required field absent from SQL: "+name));
         }
         outputPlan_=std::move(p);output_=fields;identity_=identity;return true;
      }
      catch(std::exception const& e){return std::unexpected(Err("MariaDB output",e.what()));}
   }
   template<class... Args>
      requires(adecc::db_result_type<std::remove_cvref_t<Args>>&&...)
   [[nodiscard]] framework_output_ty ExecuteOutput(std::tuple<Args...> const& values){
      if(output_.size()!=sizeof...(Args))
         return std::unexpected(Err("MariaDB output","Tuple field count mismatch"));
      adecc::db_params params;
      auto add=[&]<std::size_t I>(){
         auto const& [name,role]=output_[I];
         if(role!=adecc::db_output_param_role::identity)
            params.emplace_back(name,adecc::db_param{std::get<I>(values)},
               role!=adecc::db_output_param_role::may_be_missing);
      };
      [&]<std::size_t... I>(std::index_sequence<I...>){
         (add.template operator()<I>(),...);
      }(std::index_sequence_for<Args...>{});
      if(auto r=Run(outputPlan_,params);!r)return std::unexpected(r.error());
      try{
         std::optional<adecc::db_value> id;
         if(identity_){
            my_ulonglong raw=mysql_stmt_insert_id(stmt_.get());
            if(!raw)throw std::runtime_error{"No generated identity"};
            auto read=[&]<std::size_t I>(){
               if(*identity_!=I)return;
               using T=std::tuple_element_t<I,std::tuple<Args...>>;
               using Value=std::conditional_t<adecc::is_optional_v<T>,adecc::optional_value_type_t<T>,T>;
               if constexpr(std::integral<Value>&&!std::same_as<Value,bool>){
                  if(raw>static_cast<my_ulonglong>(std::numeric_limits<Value>::max()))
                     throw std::out_of_range{"Identity exceeds result type"};
                  id=adecc::db_value{static_cast<Value>(raw)};
               }
               else throw std::runtime_error{"Identity field must be integral"};
            };
            [&]<std::size_t... I>(std::index_sequence<I...>){
               (read.template operator()<I>(),...);
            }(std::index_sequence_for<Args...>{});
         }
         auto n=mysql_stmt_affected_rows(stmt_.get());
         return framework_output_result{
            static_cast<std::int64_t>(n==static_cast<my_ulonglong>(-1)?0:n),std::move(id)};
      }
      catch(std::exception const& e){return std::unexpected(Err("MariaDB output",e.what()));}
   }
};
template<class DB>using fw_query=query<DB>;
static_assert(framework_query_type<query,mariadb_database>);
static_assert(framework_command_query_type<query,mariadb_database>);
static_assert(framework_output_query_type<query,mariadb_database,int,std::string>);
}
