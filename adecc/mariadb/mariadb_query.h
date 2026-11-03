// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
#pragma once
/** \file mariadb_query.h \brief Typed MariaDB Connector/C query backend. */
#include "mariadb_database.h"
#include "mariadb_sql.h"
#include <mysql.h>
#include <algorithm>
#include <expected>
#include <limits>
#include <memory>
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
   std::vector<std::optional<std::string>> row_;
   std::vector<MYSQL_BIND> resultBinds_;
   std::unique_ptr<unsigned long[]> resultLengths_;
   std::unique_ptr<my_bool[]> resultNulls_,resultErrors_;
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
         std::vector<std::optional<std::string>> strings;
         for(auto const& name:plan.names) {
            auto item=std::ranges::find_if(params,[&](auto const& p){
               return detail::Equal(std::get<0>(p),name);
            });
            if(item==params.end())
               return std::unexpected(Err("MariaDB","Unbound parameter: "+name));
            strings.emplace_back(std::visit([](auto const& value){
               return detail::Encode(value);
            },std::get<1>(*item)));
         }
         std::vector<MYSQL_BIND> binds(strings.size());
         std::vector<unsigned long> lengths(strings.size());
         std::unique_ptr<my_bool[]> nulls(new my_bool[strings.size()]{});
         for(std::size_t i=0;i<strings.size();++i) {
            binds[i].buffer_type=strings[i]?MYSQL_TYPE_STRING:MYSQL_TYPE_NULL;
            nulls[i]=!strings[i];
            binds[i].is_null=&nulls[i];
            if(strings[i]){
               lengths[i]=static_cast<unsigned long>(strings[i]->size());
               binds[i].buffer=strings[i]->data();
               binds[i].buffer_length=lengths[i];
               binds[i].length=&lengths[i];
            }
         }
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
               b.buffer_type=MYSQL_TYPE_STRING;
               b.is_null=&resultNulls_[i];
               b.length=&resultLengths_[i];
               b.error=&resultErrors_[i];
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
         if(resultNulls_[i]){row_[i].reset();continue;}
         std::string text(resultLengths_[i],'\0');
         MYSQL_BIND b{};
         b.buffer_type=MYSQL_TYPE_STRING;
         b.buffer=text.data();b.buffer_length=static_cast<unsigned long>(text.size());
         unsigned long actual{};
         b.length=&actual;
         if(mysql_stmt_fetch_column(stmt_.get(),&b,static_cast<unsigned int>(i),0))
            return std::unexpected(Err("MariaDB fetch column",mysql_stmt_error(stmt_.get())));
         text.resize(actual);row_[i]=std::move(text);
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
            if(!row_[i])return std::optional<T>{};
            try{return std::optional<T>{detail::Decode<T>(*row_[i])};}
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
