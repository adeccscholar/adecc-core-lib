// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
#pragma once
/**
\file mariadb_sql.h
\brief MariaDB SQL placeholder and typed conversion helpers.
\details Only named placeholders outside quotes, comments and identifiers
are rewritten; repeated names occupy distinct question-mark positions.
*/
#include <adecc/cpp_core/value_types.h>
#include <algorithm>
#include <charconv>
#include <concepts>
#include <format>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#ifdef _WIN32
#include <windows.h>
#endif

namespace adecc::db::mariadb::detail {
struct parameter_plan {
   std::string sql;
   std::vector<std::string> names;
};
[[nodiscard]] inline bool Equal(std::string_view a,std::string_view b) {
   if(a.size()!=b.size())return false;
   for(std::size_t i=0;i<a.size();++i) {
      auto lower=[](unsigned char c)->unsigned char {
         return c>='A'&&c<='Z'?c+32:c;
      };
      if(lower(static_cast<unsigned char>(a[i]))!=lower(static_cast<unsigned char>(b[i])))
         return false;
   }
   return true;
}
[[nodiscard]] inline bool Present(parameter_plan const& p,std::string_view name) {
   return std::ranges::any_of(p.names,[&](auto const& x){return Equal(x,name);});
}
[[nodiscard]] inline bool Start(char c) {
   return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||c=='_';
}
[[nodiscard]] inline bool Cont(char c) {
   return Start(c)||(c>='0'&&c<='9');
}
/**
\brief Build the physical ? binding plan.
\details Handles SQL single/double quotes, backtick-quoted identifiers,
MySQL # and -- comments, C comments and backslash escapes.
\param sql Source adecc SQL using :Name.
\returns Rewritten SQL and ordered parameter names.
\throw std::invalid_argument On unsupported positional placeholders or malformed input.
*/
[[nodiscard]] inline parameter_plan Plan(std::string_view sql) {
   enum class state {normal,single,dbl,identifier,line,block};
   state mode=state::normal;
   parameter_plan out;out.sql.reserve(sql.size());
   for(std::size_t i=0;i<sql.size();) {
      char const c=sql[i],next=i+1<sql.size()?sql[i+1]:'\0';
      if(mode==state::normal) {
         if(c=='\''||c=='"'||c==static_cast<char>(96)) {
            mode=c=='\''?state::single:c=='"'?state::dbl:state::identifier;
            out.sql+=c;++i;continue;
         }
         if(c=='#'||(c=='-'&&next=='-'&&
             (i+2>=sql.size()||static_cast<unsigned char>(sql[i+2])<=32))) {
            mode=state::line;out.sql+=c;++i;continue;
         }
         if(c=='/'&&next=='*') {
            mode=state::block;out.sql+="/*";i+=2;continue;
         }
         if(c==':'&&next==':') {out.sql+="::";i+=2;continue;}
         if(c==':'&&Start(next)) {
            std::size_t end=i+2;
            while(end<sql.size()&&Cont(sql[end]))++end;
            out.names.emplace_back(sql.substr(i+1,end-i-1));
            out.sql+='?';i=end;continue;
         }
         if(c=='?')throw std::invalid_argument{"Positional ? placeholders are not part of the adecc SQL contract"};
      }
      else if(mode==state::line) {
         if(c=='\n'||c=='\r')mode=state::normal;
      }
      else if(mode==state::block) {
         if(c=='*'&&next=='/') {out.sql+="*/";i+=2;mode=state::normal;continue;}
      }
      else {
         char const terminator=mode==state::single?'\'':mode==state::dbl?'"':static_cast<char>(96);
         if(c=='\\'&&i+1<sql.size()) {
            out.sql+=c;out.sql+=next;i+=2;continue;
         }
         if(c==terminator) {
            if(next==terminator) {
               out.sql+=c;out.sql+=next;i+=2;continue;
            }
            mode=state::normal;
         }
      }
      out.sql+=c;++i;
   }
   if(mode==state::single||mode==state::dbl||mode==state::identifier||mode==state::block)
      throw std::invalid_argument{"Unterminated SQL string, identifier or comment"};
   return out;
}

[[nodiscard]] inline std::string WideToUtf8(std::wstring_view v) {
#ifdef _WIN32
   if(v.empty())return {};
   int const n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,
      v.data(),static_cast<int>(v.size()),nullptr,0,nullptr,nullptr);
   if(n<=0)throw std::runtime_error{"Invalid UTF-16 input"};
   std::string out(static_cast<std::size_t>(n),'\0');
   if(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,
      v.data(),static_cast<int>(v.size()),out.data(),n,nullptr,nullptr)!=n)
      throw std::runtime_error{"UTF-8 conversion failed"};
   return out;
#else
   throw std::runtime_error{"MariaDB wide strings require Windows"};
#endif
}
[[nodiscard]] inline std::wstring Utf8ToWide(std::string_view v) {
#ifdef _WIN32
   if(v.empty())return {};
   int const n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,
      v.data(),static_cast<int>(v.size()),nullptr,0);
   if(n<=0)throw std::runtime_error{"Invalid UTF-8 input"};
   std::wstring out(static_cast<std::size_t>(n),L'\0');
   if(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,
      v.data(),static_cast<int>(v.size()),out.data(),n)!=n)
      throw std::runtime_error{"UTF-8 decoding failed"};
   return out;
#else
   throw std::runtime_error{"MariaDB wide strings require Windows"};
#endif
}
template<class T>
[[nodiscard]] std::optional<std::string> Encode(T const& value) {
   if constexpr(adecc::is_optional_v<T>) {
      if(!value)return std::nullopt;
      return Encode(*value);
   }
   else if constexpr(std::same_as<T,std::string>||std::same_as<T,std::string_view>)
      return std::string{value};
   else if constexpr(std::same_as<T,char const*>)
      return value?std::optional<std::string>{value}:std::nullopt;
   else if constexpr(std::same_as<T,std::wstring>||std::same_as<T,std::wstring_view>)
      return WideToUtf8(value);
   else if constexpr(std::same_as<T,wchar_t const*>)
      return value?std::optional<std::string>{WideToUtf8(value)}:std::nullopt;
   else if constexpr(std::same_as<T,bool>)return value?"1":"0";
   else if constexpr(std::integral<T>)return std::to_string(value);
   else if constexpr(std::same_as<T,double>)return std::format("{:.17g}",value);
   else return adecc::ConvertTo<std::string>(value);
}
template<class T>
[[nodiscard]] T Decode(std::string const& text) {
   if constexpr(std::same_as<T,std::string>)return text;
   else if constexpr(std::same_as<T,std::wstring>)return Utf8ToWide(text);
   else if constexpr(std::same_as<T,bool>) {
      if(text=="1")return true;
      if(text=="0")return false;
      throw std::runtime_error{"Invalid MariaDB boolean"};
   }
   else if constexpr(std::integral<T>) {
      T result{};
      auto [end,ec]=std::from_chars(text.data(),text.data()+text.size(),result);
      if(ec!=std::errc{}||end!=text.data()+text.size())
         throw std::runtime_error{"Invalid MariaDB integer"};
      return result;
   }
   else if constexpr(std::same_as<T,double>) {
      double result{};
      auto [end,ec]=std::from_chars(text.data(),text.data()+text.size(),result);
      if(ec!=std::errc{}||end!=text.data()+text.size())
         throw std::runtime_error{"Invalid MariaDB double"};
      return result;
   }
   else if constexpr(std::same_as<T,adecc::timestamp_ty>) {
      if(text.size()<19||(text[10]!=' '&&text[10]!='T'))
         throw std::runtime_error{"Invalid MariaDB timestamp"};
      std::string out=text.substr(0,19);out[10]='T';
      return adecc::ConvertTo<T>(out);
   }
   else if constexpr(std::same_as<T,adecc::time_ty>) {
      if(text.size()<8)throw std::runtime_error{"Invalid MariaDB time"};
      return adecc::ConvertTo<T>(text.substr(0,8));
   }
   else return adecc::ConvertTo<T>(text);
}
}
