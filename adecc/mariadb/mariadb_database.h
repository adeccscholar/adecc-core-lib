// SPDX-FileCopyrightText: 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0
#pragma once
/**
\file mariadb_database.h
\brief MariaDB Connector/C physical database boundary for adecc-core-lib.
\details The connection is copyable through reconnect semantics and movable
through ownership transfer. A serverless test validates the Concepts.
*/
#include <adecc/cpp_core/database_definitions.h>
#include <mysql.h>
#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <utility>

namespace adecc::db::mariadb {
   struct mariadb_credentials {
      std::string strHost{"localhost"};
      std::uint16_t uPort{3306};
      std::string strDatabase{"BuildEngineTest"};
      std::string strUser{"buildenginetest_user"};
      std::string strPassword;
      std::string strCaFile;
      bool boRequireTls{true};
   };

   /** \brief Physical, RAII-owned MariaDB database connection.
   \details Error messages never include credentials.
   */
   class mariadb_database {
      struct deleter {
         void operator()(MYSQL* p) const noexcept { if(p) mysql_close(p); }
      };
      mariadb_credentials aCredentials{};
      std::unique_ptr<MYSQL,deleter> pConnection;
      bool boTransaction{};
      [[nodiscard]] static framework_result_ty Error(std::string strMessage,
         std::string strDetails) {
         return std::unexpected(error_ty{{},std::move(strMessage),std::move(strDetails)});
      }
   public:
      mariadb_database()=default;
      explicit mariadb_database(mariadb_credentials credentials)
         : aCredentials(std::move(credentials)) {}
      mariadb_database(mariadb_database const& rhs)
         : aCredentials(rhs.aCredentials) {
         if(rhs.Connected()) (void)Connect();
      }
      mariadb_database& operator=(mariadb_database const& rhs) {
         if(this!=&rhs) {
            pConnection.reset();
            boTransaction=false;
            aCredentials=rhs.aCredentials;
            if(rhs.Connected()) (void)Connect();
         }
         return *this;
      }
      mariadb_database(mariadb_database&&)=default;
      mariadb_database& operator=(mariadb_database&&)=default;
      [[nodiscard]] std::string GetServer() const {
         return std::format("{}:{}",aCredentials.strHost,aCredentials.uPort);
      }
      [[nodiscard]] std::string GetInformation() const {
         return std::format("DriverID=MariaDB/Connector-C; Database={}; User={}; TLS={}",
            aCredentials.strDatabase,aCredentials.strUser,aCredentials.boRequireTls);
      }
      [[nodiscard]] bool Connected() const { return pConnection!=nullptr; }
      [[nodiscard]] MYSQL* NativeHandle() const noexcept { return pConnection.get(); }
      [[nodiscard]] framework_result_ty Connect(mariadb_credentials const& credentials) {
         aCredentials=credentials;
         return Connect();
      }
      [[nodiscard]] framework_result_ty Connect() {
         pConnection.reset();
         boTransaction=false;
         MYSQL* pRaw=mysql_init(nullptr);
         if(!pRaw) return Error("MariaDB connect","mysql_init failed");
         std::unique_ptr<MYSQL,deleter> pCandidate{pRaw};
         if(mysql_options(pRaw,MYSQL_SET_CHARSET_NAME,"utf8mb4")!=0)
            return Error("MariaDB charset",mysql_error(pRaw));
         if(aCredentials.boRequireTls) {
            if(mysql_ssl_set(pRaw,nullptr,nullptr,
               aCredentials.strCaFile.empty()?nullptr:aCredentials.strCaFile.c_str(),
               nullptr,nullptr)!=0)
               return Error("MariaDB TLS",mysql_error(pRaw));
            my_bool bVerify=1;
            if(mysql_options(pRaw,MYSQL_OPT_SSL_VERIFY_SERVER_CERT,&bVerify)!=0)
               return Error("MariaDB TLS verification",mysql_error(pRaw));
         }
         if(!mysql_real_connect(pRaw,aCredentials.strHost.c_str(),
            aCredentials.strUser.c_str(),aCredentials.strPassword.c_str(),
            aCredentials.strDatabase.c_str(),aCredentials.uPort,nullptr,0))
            return Error("MariaDB connect",mysql_error(pRaw));
         pConnection=std::move(pCandidate);
         return true;
      }
      [[nodiscard]] framework_result_ty BeginTransaction() {
         if(!Connected())return Error("BeginTransaction","Not connected");
         if(boTransaction)return Error("BeginTransaction","Transaction already active");
         if(mysql_autocommit(pConnection.get(),0)!=0)
            return Error("BeginTransaction",mysql_error(pConnection.get()));
         boTransaction=true;
         return true;
      }
      [[nodiscard]] framework_result_ty Commit() {
         if(!Connected()||!boTransaction)return Error("Commit","No active transaction");
         if(mysql_commit(pConnection.get())!=0)
            return Error("Commit",mysql_error(pConnection.get()));
         if(mysql_autocommit(pConnection.get(),1)!=0)
            return Error("Commit",mysql_error(pConnection.get()));
         boTransaction=false;
         return true;
      }
      [[nodiscard]] framework_result_ty Rollback() {
         if(!Connected()||!boTransaction)return Error("Rollback","No active transaction");
         if(mysql_rollback(pConnection.get())!=0)
            return Error("Rollback",mysql_error(pConnection.get()));
         if(mysql_autocommit(pConnection.get(),1)!=0)
            return Error("Rollback",mysql_error(pConnection.get()));
         boTransaction=false;
         return true;
      }
   };

   static_assert(framework_database_type<mariadb_database>);
   static_assert(framework_database_with_credentials<mariadb_database,mariadb_credentials>);
}
