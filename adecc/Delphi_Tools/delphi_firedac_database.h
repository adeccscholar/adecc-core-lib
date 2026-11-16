#pragma once

#include <cctype>
#include <algorithm>
#include <ranges>



#include "tools.h"
#include "database_definitions.h"
#include "database_exception.h"

#include "delphi_convert.h"

#include <System.Classes.hpp>
#include <Data.DB.hpp>
#include <FireDAC.Comp.Client.hpp>
#include <FireDAC.Comp.DataSet.hpp>
#include <FireDAC.Comp.UI.hpp>
#include <FireDAC.DApt.hpp>
#include <FireDAC.DApt.Intf.hpp>
#include <FireDAC.DatS.hpp>
#include <FireDAC.Phys.hpp>
#include <FireDAC.Phys.Intf.hpp>
#include <FireDAC.Phys.MSSQL.hpp>
#include <FireDAC.Phys.MSSQLDef.hpp>
#include <FireDAC.Stan.Async.hpp>
#include <FireDAC.Stan.Def.hpp>
#include <FireDAC.Stan.Error.hpp>
#include <FireDAC.Stan.Intf.hpp>
#include <FireDAC.Stan.Option.hpp>
#include <FireDAC.Stan.Param.hpp>
#include <FireDAC.Stan.Pool.hpp>
#include <FireDAC.UI.Intf.hpp>

#if defined BUILD_WITH_FMX
#include <FireDAC.FMXUI.Wait.hpp>
#endif

#if defined BUILD_WITH_VCL
#include <FireDAC.VCLUI.Wait.hpp>
#endif


#include <string>
#include <sstream>
#include <memory>
#include <tuple>
#include <optional>
#include <variant>
#include <chrono>
#include <cstdint>
#include <type_traits>
#include <utility>

using namespace std::string_literals;

namespace adecc::db::delphi {

struct query_del { void operator()(TFDQuery* p) const noexcept { if(p) { p->Close(); delete p; } } };
struct conn_del { void operator()(TFDConnection* p) const noexcept { if(p) { p->Close(); delete p; } } };

// ---------------------------------------------------------------
// Konkrete MS SQL FireDAC-Datenbank: db_database_type-konform
// ---------------------------------------------------------------
/*!
\brief MS SQL Datenbank (FireDAC) mit konzept-konformer Oberfläche
\details
   \note Erfüllt \c db_database_type:
      - Default-/Copy-/Move-Konstruktion
      - \c Connect() und \c Connect(server,db,integrated,user,pwd)
      - \c Connected(), \c BeginTransaction(), \c Commit(), \c Rollback()
      - \c GetInformation()
*/
class mssql_database {
private:
   std::string strServer{};
   std::string strDatabase{};
   bool        boIntegrated{true};
   std::string strUser{};
   std::string strPassKey{};
   bool        boMARS = true;  // Multiple Active Result Sets (MARS): Verwenden mehrerer aktiver Resultsets in SQL Server Native Client
   int         iTimeout { 30 }; // Login Timeout in seconds

   std::unique_ptr<TFDConnection, conn_del> theConnection{ nullptr };

public:
   // -------------------- Konstruktion / Zuweisung --------------------
   mssql_database() = default;

   mssql_database(std::string const& svr, std::string const& db, bool integrated = true,
                          std::string const& usr = {}, std::string const& pwd = {})
      : strServer{svr}, strDatabase{db}, boIntegrated{integrated}, strUser{usr}, strPassKey{pwd} {
      BuildConnection_(); // wirft bei Fehler
      }

   // Copy: baut neue Connection mit denselben Credentials auf
   mssql_database(mssql_database const& rhs)
      : strServer{rhs.strServer}
      , strDatabase{rhs.strDatabase}
      , boIntegrated{rhs.boIntegrated}
      , strUser{rhs.strUser}
      , strPassKey{rhs.strPassKey} {
      if (!strServer.empty() && !strDatabase.empty()) {
         BuildConnection_(); // frische Verbindung für die Kopie
         }
      }

   mssql_database& operator=(mssql_database const& rhs) {
      if (this != &rhs) {
         Close_();
         strServer    = rhs.strServer;
         strDatabase  = rhs.strDatabase;
         boIntegrated = rhs.boIntegrated;
         strUser      = rhs.strUser;
         strPassKey   = rhs.strPassKey;
         if (!strServer.empty() && !strDatabase.empty()) {
            BuildConnection_();
            }
         }
      return *this;
      }

   // Move: überträgt Besitz
   mssql_database(mssql_database&&) noexcept = default;
   mssql_database& operator=(mssql_database&&) noexcept = default;

   ~mssql_database() = default;

   // -------------------- Zugriff / Info --------------------
   std::string const& Server()   const { return strServer; }
   std::string const& Database() const { return strDatabase; }
   bool               Integrated() const { return boIntegrated; }
   std::string const& UserName()  const { return strUser; }
   std::string const& Password()  const { return strPassKey; }

   operator TFDConnection* () { return theConnection.get(); }
   operator TFDConnection const* () const { return theConnection.get(); }

   /*!
   \brief Liefert eine Parameter-Stringrepräsentation (INI-Stil) der Verbindung
   */
   operator std::string () const {
      return GetInformation();
      }

   std::string GetServer() const {
	  return std::format("{}:{}", strServer, strDatabase);
      }

   /*!
   \brief Geforderte Konzept-Methode: Verbindungsinformationen
   */
   std::string GetInformation() const {
      std::ostringstream os;
      os << "DriverID=MSSQL\n";
      if (Server() == ".") os << "Server=(local)\n";
      else                 os << "Server=" << Server() << "\n";
      os << "Database=" << Database() << "\n";
      if (Integrated()) {
         os << "OsAuthent=Yes\n"
            << "User_Name=\"\"\n"
            << "Password=\"\"\n";
         }
      else {
         os << "OsAuthent=No\n"
            << "User_Name=" << UserName() << "\n"
            << "Password="  << Password() << "\n";
         }
      os << std::format("MARS={}\n", boMARS ? "Yes" : "No")
         << std::format("LoginTimeout={}\n", iTimeout)
         << "VariantFormat=String\n";
      return os.str();
      }

   // Asynchronous Processing=True;
   // Packet Size=4096 (8192 ist Standard)
   // Column Encryption Setting=enabled
   //   + Enclave Attestation Url=http://hgs.bastion.local/Attestation;

   // -------------------- Konzept-API: Connect/Status --------------------
   /*!
   \brief Konzept-Methode: Verbinde mit gespeicherten Credentials (Re-Connect)
   \returns true bei Erfolg
   */
   framework_result_ty Connect() {
      try {
         // Bereits offen? Dann kurz schließen und neu öffnen (reconnect)
         if (theConnection && theConnection->Connected) {
            theConnection->Close();
            }
         // Falls noch kein Handle, neu bauen
         if (!theConnection) {
            BuildConnection_();
            return true;
            }
         // Handle vorhanden: Parameter neu setzen und öffnen
         ApplyParamsAndOpen_(*theConnection);
         return true;
         }
      catch (Exception const& ex) {
		 return std::unexpected(error_ty { {}, std::string{"database not connected"},
										   ConvertTo<std::string>(ex.Message) });
		 }
      }

   /*!
   \brief Konzept-Methode: Verbinde mit expliziten Credentials
   \returns true bei Erfolg
   */
   framework_result_ty Connect(std::string const& svr, std::string const& db, bool integrated = true,
                std::string const& usr = {}, std::string const& pwd = {}) {
      strServer    = svr;
      strDatabase  = db;
      boIntegrated = integrated;
      strUser      = usr;
      strPassKey   = pwd;
      try {
         if (!theConnection) {
            BuildConnection_();
            }
         else {
            if (theConnection->Connected) {
               theConnection->Close();
               }
            ApplyParamsAndOpen_(*theConnection);
            }
         return true;
         }
      catch (Exception const& ex) {
         return std::unexpected(error_ty { {}, std::string{"database not connected"},
                                           ConvertTo<std::string>(ex.Message) });
         }
      }

   /*!
   \brief Konzept-Methode: Verbindungsstatus
   */
   bool Connected() const {
      return theConnection && theConnection->Connected;
      }

   // -------------------- Konzept-API: Transaktionen --------------------
   /*!
   \brief Startet eine Transaktion
   */
   framework_result_ty BeginTransaction() {
      if (!Connected()) return false;
      try {
         theConnection->StartTransaction();
         return true;
         }
      catch (Exception const& ex) {
         return std::unexpected(error_ty { {}, std::string{"Begin Transaction failed"},
                                           ConvertTo<std::string>(ex.Message) });
         }
      }

   /**
   \brief Bestätigt die Transaktion
   */
   framework_result_ty Commit() {
      if (!Connected()) return false;
      try {
         theConnection->Commit();
         return true;
         }
      catch (Exception const& ex) {
         return std::unexpected(error_ty { {}, std::string{"Commit Transaction failed"},
                                           ConvertTo<std::string>(ex.Message) });
         }
      }

   /*!
   \brief Rollback der Transaktion
   */
   framework_result_ty Rollback() {
      if (!Connected()) return false;
      try {
         theConnection->Rollback();
         return true;
         }
      catch (Exception const& ex) {
         return std::unexpected(error_ty { {}, std::string{"Rollback Transaction failed"},
                                           ConvertTo<std::string>(ex.Message) });
         }
      }

private:
   // -------------------- interner Aufbau --------------------
   void Close_() noexcept {
      if (theConnection) {
         try {
            if (theConnection->Connected) {
               theConnection->Close();
               }
            }
         catch (...) {
            }
         theConnection.reset();
         }
      }


   void BuildConnection_(ExceptionInformation const& call_info = { "Called from",  src_loc::current(),
                                                                   std::chrono::system_clock::now() }) {
      theConnection = adecc::make_unique_with_deleter<TFDConnection>(conn_del{}, nullptr /* Owner */);
      if (!theConnection) {
         throw StandardError<database_exception>(call_info.renew("Exception throw at"),
                                                 in_place_exception,
                                                 "can't create framework database server",
                                                 GetServer(), GetInformation(),
                                                 "can't create TFDConnection");
         }
      ApplyParamsAndOpen_(*theConnection, call_info.renew("Called from") );
      }

   void ApplyParamsAndOpen_(TFDConnection& conn,
                            ExceptionInformation const& call_info = { "Called from",  src_loc::current(),
                                                                   std::chrono::system_clock::now() }) {
      conn.Params->Clear();

      auto AddParam = [&conn](std::string const& param) {
         conn.Params->Add(param.c_str());
         };

      AddParam("DriverID=MSSQL");
      AddParam("Server=" + std::string{ (Server() == "." ? "(local)" : Server()) });
      AddParam("Database=" + Database());

      if (Integrated()) {
         AddParam("OsAuthent=Yes");
         AddParam("User_Name=\"\"");
         AddParam("Password=\"\"");
         conn.LoginPrompt = false;
         }
      else {
         AddParam("OsAuthent=No");
         AddParam("User_Name=" + UserName());
         AddParam("Password="  + Password());
         conn.LoginPrompt = (UserName().empty() || Password().empty());
         }

      AddParam(std::format("MARS={}", boMARS ? "Yes" : "No").c_str());
      AddParam(std::format("LoginTimeout={}", iTimeout).c_str());
      AddParam("VariantFormat=String");
      AddParam("Encrypt=No");
      // AddParam("Pooled=True");      // optional: Pooling

      try {
         conn.Open(); // wirft FireDAC-Exception bei Fehler
         }
      catch(Exception const& ex) {
         throw StandardError<database_exception>(call_info.renew("Exception at"), in_place_exception,
                                                 "can't create framework database server",
                                                 GetServer(), GetInformation(),
                                                 ConvertTo<std::string>(ex.Message));
         }
      }
};

static_assert(adecc::db::framework_database_type<mssql_database>);


// ------------------------------------------------------------------
// FireDAC Framework-Query (Template über DB, Concept-gesichert)
// ------------------------------------------------------------------
template <adecc::db::framework_database_type db_ty>
class query {
public:
   using fw_query_type = TFDQuery*;

   explicit query(db_ty const& aDb)
      : pDbConn{ static_cast<TFDConnection*>(const_cast<db_ty&>(aDb)) }
      , upQry{ adecc::make_unique_with_deleter<TFDQuery>(query_del{}, /*Owner*/ nullptr) } {
      if (!pDbConn) {
        throw StandardError<database_exception>({}, in_place_exception,
                                                 "can't create framework query",
                                                 aDb.GetServer(), aDb.GetInformation(),
                                                 "No valid database connection");


         }

      if (!upQry) {
        throw StandardError<database_exception>({}, in_place_exception,
                                                 "can't create framework query",
                                                 aDb.GetServer(), aDb.GetInformation(),
                                                 "Query couldn't be created");
         }

      upQry->Connection = pDbConn;
      upQry->ResourceOptions->SilentMode = true;
      upQry->UpdateOptions->UpdateMode = TUpdateMode::upWhereAll;
      }

   framework_result_ty SetSql(std::string const& sql) {
      try {
         upQry->SQL->Clear();
         upQry->SQL->Add(sql.c_str());
         return true;
         }
      catch (Exception const& ex) {
		 return std::unexpected(error_ty { {}, "error for set sql in query",
										   ConvertTo<std::string>(ex.Message) });           }
      }

   framework_result_ty Open(std::string const& sql, adecc::db_params const& params) {
      if (auto setq = SetSql(sql); !setq) [[unlikely]] {
         auto const& [info, msg, details] = setq.error();
         return std::unexpected(error_ty { info.renew("Call Function"), "error for open the query",
                                           details });
         }

      if (auto bndp = BindAllParams_(params); !bndp) [[unlikely]] {
         auto const& [info, msg, details] = bndp.error();
         return std::unexpected(error_ty { info.renew("Call Function"), "error for open the query",
                                           details });
         }

      return OpenOrExecute_();
      }

   framework_result_ty Open(adecc::db_params const& params) {
      if (auto bndp = BindAllParams_(params); !bndp) [[unlikely]] {
         auto const& [info, msg, details] = bndp.error();
         return std::unexpected(error_ty { info.renew("Call Function"), "error for open the query",
                                           details });
         }

      return OpenOrExecute_();
      }

   /*!
   \brief Führt eine SQL-Anweisung ohne Parameter und ohne Resultset aus
   \details
      Diese Methode ist für DDL und einfache DML gedacht. Sie nutzt bewusst
      nicht den Output-Parameterpfad.

   \param strSql SQL-Anweisung
   \returns Anzahl der betroffenen Zeilen und keine Identity
   */
   framework_output_ty ExecuteCommand(std::string const& strSql) {
      if (auto setq = SetSql(strSql); !setq) [[unlikely]] {
         auto const& [info, msg, details] = setq.error();
         return std::unexpected(error_ty { info.renew("Call Function"),
                                           "error for execute command",
                                           details });
         }

      try {
         upQry->ExecSQL();
         return framework_output_result {
            static_cast<std::int64_t>(upQry->RowsAffected),
            std::nullopt
            };
         }
      catch (Exception const& ex) {
         return std::unexpected(error_ty { {}, "error for execute command",
                                           ConvertTo<std::string>(ex.Message) });
         }
      }

   /*!
   \brief Bereitet eine Output-Ausführung vor
   \details
      Die SQL-Anweisung wird wie im bestehenden FireDAC-Pfad in das Query-Objekt
      übernommen. FireDAC erzeugt dabei die Parameterliste, die spätere
      Ausführung bindet die konkreten Tuple-Werte über den bewährten
      \c db_params Bindepfad.

      \note
      Diese Implementierung ist zunächst für MS SQL ausgelegt. Oracle benötigt
      später gegebenenfalls \c RETURNING \c ID \c INTO mit einem OUT-Parameter.
      Interbase benötigt gegebenenfalls \c RETURNING \c ID und das Lesen eines
      Resultsets.

   \param strSql SQL-Anweisung
   \param vecOutputParams Output-Parameterbeschreibung
   \returns true bei erfolgreicher Vorbereitung
   */
   framework_result_ty PrepareOutput(
      std::string const& strSql,
      db_output_parameters const& vecOutputParams
   ) {
      strOutputSql = strSql;
      vecOutputParameters = vecOutputParams;

      boHasIdentityOutput = false;
      for (auto const& aParam : vecOutputParameters) {
         if (std::get<1>(aParam) == db_output_param_role::identity) {
            boHasIdentityOutput = true;
            break;
            }
         }

      if (auto setq = SetSql(strOutputSql); !setq) [[unlikely]] {
         auto const& [info, msg, details] = setq.error();
         return std::unexpected(error_ty { info.renew("Call Function"),
                                           "error for prepare output query",
                                           details });
         }

      return true;
      }

   /*!
   \brief Führt die vorbereitete Output-Anweisung für ein Tuple aus
   \details
      Das Tuple wird nicht verändert und nicht konsumiert. Die Bindung wird
      über eine temporäre \c db_params Liste auf den vorhandenen und
      funktionierenden FireDAC-Bindepfad abgebildet.

   \tparam Args Typen des Tuple-Werts
   \param tupValues Werte des aktuellen Range-Elements
   \returns Tuple aus betroffenen Zeilen und optionalem Identity-Wert
   */
   template <class... Args>
      requires (adecc::db_result_type<std::remove_cvref_t<Args>> && ...)
   framework_output_ty ExecuteOutput(std::tuple<Args...> const& tupValues) {
      if (vecOutputParameters.size() != sizeof...(Args)) {
         return std::unexpected(error_ty { {},
                                           "error for execute output query",
                                           std::format("output parameter count {} does not match tuple size {}",
                                                       vecOutputParameters.size(),
                                                       sizeof...(Args)) });
         }

      auto optParams = MakeOutputParams_(tupValues);
      if (!optParams) [[unlikely]] {
         auto const& [info, msg, details] = optParams.error();
         return std::unexpected(error_ty { info.renew("Call Function"),
                                           "error for execute output query",
                                           details });
         }

      if (auto bndp = BindAllParams_(*optParams); !bndp) [[unlikely]] {
         auto const& [info, msg, details] = bndp.error();
         return std::unexpected(error_ty { info.renew("Call Function"),
                                           "error for execute output query",
                                           details });
         }

      try {
         upQry->ExecSQL();
         return framework_output_result {
            static_cast<std::int64_t>(upQry->RowsAffected),
            boHasIdentityOutput ? ReadIdentity_() : std::optional<db_value>{}
            };
         }
      catch (Exception const& ex) {
         return std::unexpected(error_ty { {}, "error for execute output query",
                                           ConvertTo<std::string>(ex.Message) });
         }
      }

   framework_result_ty First() {
      try {
         if (!upQry || !upQry->Active) return false;
         upQry->First();
         return !upQry->Eof;
         }
      catch (Exception const& ex) {
         return std::unexpected(error_ty { {}, "error for going to the first dataset",
											  ConvertTo<std::string>(ex.Message) });
         }
      }

   framework_result_ty Fetch() {
      if (!upQry || !upQry->Active) return false;
      try {
         upQry->Next();
         return !upQry->Eof;
         }
      catch (Exception const& ex) {
         return std::unexpected(error_ty { {}, "error for fetching the next dataset",
											  ConvertTo<std::string>(ex.Message) });
         }
      }

   framework_result_ty Eof() const {
      try {
         return !upQry || !upQry->Active || upQry->Eof;
         }
      catch (Exception const& ex) {
         return std::unexpected(error_ty { {}, "error for checking end of dataset",
											  ConvertTo<std::string>(ex.Message) });
         }
      }

   // ---------------------------------------------------------------
   // NEU: Templated GetField<ty> – ty muss in my_defined_types liegen
   // ---------------------------------------------------------------
   template <class ty> requires adecc::is_in_type_list_v<ty, adecc::defined_values_types>
   framework_get_ty<ty> GetField(std::string const& field) const {
      if (!upQry || !upQry->Active) {
		 return std::unexpected(error_ty { {}, std::format("error for get the field '{}'", field),
										   "query don't exists or isn't avtive." });
		 }
	  else [[likely]] {
		 TField* fld = nullptr;
		 try {
			fld = upQry->FindField(field.c_str());
			}
		 catch (Exception const& ex) {
			return std::unexpected(error_ty { {}, std::format("error for get the field '{}'", field),
											  ConvertTo<std::string>(ex.Message) });
			}
		 if (!fld || fld->IsNull) {
            return std::nullopt;
			}
         else {
		    Variant const& v = fld->Value;
		    return adecc::ConvertToOpt<ty>(v); // nutzt Konvertierungsschicht
            }
		 }
	  }


   std::vector<std::string> GetAttributes() const {
      std::vector<std::string> names;
      if (!upQry || !upQry->Active) return names;
      names.reserve(upQry->Fields->Count);
      for (int i = 0; i < upQry->Fields->Count; ++i) {
         names.emplace_back(AnsiString(upQry->Fields->Fields[i]->FieldName).c_str());
         }
      return names;
      }

   fw_query_type NativeHandle() const noexcept { return upQry.get(); }

private:

   static std::string TrimLeftLower_(std::string const& strValue) {
      auto it = std::ranges::find_if_not(strValue, [](unsigned char const ch) {
         return std::isspace(ch) != 0;
         });

      std::string strResult{ it, strValue.end() };
      std::ranges::transform(strResult, strResult.begin(), [](unsigned char const ch) {
         return static_cast<char>(std::tolower(ch));
         });

      return strResult;
      }

   static bool IsOpenStatement_(std::string const& strSql) {
      auto const strLower = TrimLeftLower_(strSql);

      return strLower.starts_with("select") || strLower.starts_with("with");
      }

   framework_result_ty OpenOrExecute_() {
      try {
         auto const strSql = ConvertTo<std::string>(upQry->SQL->Text);

         if (IsOpenStatement_(strSql)) {
            upQry->Open();
            }
         else {
            upQry->ExecSQL();
            }

         return true;
         }
      catch (Exception const& ex) {
         return std::unexpected(error_ty { {}, "error for open the query",
                                           ConvertTo<std::string>(ex.Message) });
         }
      }

   template <class ty>
   static TFieldType FireDacFieldType_() {
      using clean_ty = std::remove_cvref_t<ty>;

      if constexpr (std::is_same_v<clean_ty, std::string> || std::is_same_v<clean_ty, std::string_view> ||
                    std::is_same_v<clean_ty, char const*> || std::is_same_v<clean_ty, wchar_t const*>) {
         return Data::Db::ftWideString;
         }
      else if constexpr (std::is_same_v<clean_ty, int> || std::is_same_v<clean_ty, unsigned int>) {
         return Data::Db::ftInteger;
         }
      else if constexpr (std::is_same_v<clean_ty, long long> || std::is_same_v<clean_ty, unsigned long long>) {
         return Data::Db::ftLargeint;
         }
      else if constexpr (std::is_same_v<clean_ty, double> || std::is_same_v<clean_ty, money_ty>) {
         return Data::Db::ftFloat;
         }
      else if constexpr (std::is_same_v<clean_ty, bool>) {
         return Data::Db::ftBoolean;
         }
      else if constexpr (std::is_same_v<clean_ty, adecc::date_ty>) {
         return Data::Db::ftDate;
         }
      else if constexpr (std::is_same_v<clean_ty, adecc::timestamp_ty>) {
         return Data::Db::ftDateTime;
         }
      else if constexpr (std::is_same_v<clean_ty, adecc::time_ty>) {
         return Data::Db::ftTime;
         }
      else {
         static_assert(sizeof(clean_ty) == 0, "Unsupported FireDAC parameter type");
         }
      }

   /*!
   \brief Prüft, ob ein Parameter im FireDAC Query vorhanden ist
   \details FireDAC verwendet Parameternamen ohne führenden Doppelpunkt.
   */
   bool ContainsParam_(std::string const& strName) const {
      if (!upQry || !upQry->Params) {
         return false;
         }

      return upQry->Params->FindParam(strName.c_str()) != nullptr;
      }

   template <class ty>
   static db_param MakeDbParamValue_(ty const& aValue) {
      return db_param { aValue };
      }

   template <class... Args>
   std::expected<db_params, error_ty> MakeOutputParams_(
      std::tuple<Args...> const& tupValues
   ) const {
      db_params vecParams;
      vecParams.reserve(sizeof...(Args));

      auto aResult = MakeOutputParamsImpl_(
         vecParams,
         tupValues,
         std::index_sequence_for<Args...>{}
         );

      if (!aResult) {
         auto const& [info, msg, details] = aResult.error();
         return std::unexpected(error_ty { info, msg, details });
         }

      return vecParams;
      }

   template <class... Args, std::size_t... Is>
   framework_result_ty MakeOutputParamsImpl_(
      db_params& vecParams,
      std::tuple<Args...> const& tupValues,
      std::index_sequence<Is...>
   ) const {
      framework_result_ty aResult{ true };

      auto fnAdd = [&]<std::size_t I>(std::integral_constant<std::size_t, I>) {
         if (!aResult) {
            return;
            }

         auto const& [strName, aRole] = vecOutputParameters[I];

         switch (aRole) {
         case db_output_param_role::needed_value:
         case db_output_param_role::key:
            if (!ContainsParam_(strName)) {
               aResult = std::unexpected(error_ty { {},
                                                    "error for bind output parameter",
                                                    std::format("required output parameter '{}' is not used in SQL", strName) });
               return;
               }
            vecParams.emplace_back(
               strName,
               MakeDbParamValue_(std::get<I>(tupValues)),
               true
               );
            break;

         case db_output_param_role::may_be_missing:
            if (ContainsParam_(strName)) {
               vecParams.emplace_back(
                  strName,
                  MakeDbParamValue_(std::get<I>(tupValues)),
                  false
                  );
               }
            break;

         case db_output_param_role::identity:
            break;
            }
         };

      (fnAdd(std::integral_constant<std::size_t, Is>{}), ...);

      return aResult;
      }

   /*!
   \brief Liest den zuletzt erzeugten Identity-Wert für MS SQL
   \details
      Diese Implementierung nutzt FireDAC \c GetLastAutoGenValue und ist für
      MS SQL vorgesehen. Der Aufruf erfolgt nur, wenn eine Output-Parameterrolle
      \c identity vorhanden ist.

      \note
      Oracle benötigt später gegebenenfalls \c RETURNING \c ID \c INTO mit
      einem OUT-Parameter. Interbase benötigt gegebenenfalls \c RETURNING \c ID
      und das Lesen eines Resultsets.
   */
   std::optional<db_value> ReadIdentity_() const {
      if (!pDbConn) {
         return std::nullopt;
         }

      try {
         System::Variant const aValue = pDbConn->GetLastAutoGenValue(System::UnicodeString());

         if (System::Variants::VarIsEmpty(aValue) ||
             System::Variants::VarIsNull(aValue)) {
            return std::nullopt;
            }

         if (auto optValue = adecc::ConvertToOpt<int>(aValue); optValue) {
            return db_value { *optValue };
            }

         if (auto optValue = adecc::ConvertToOpt<long long>(aValue); optValue) {
            return db_value { *optValue };
            }

         if (auto optValue = adecc::ConvertToOpt<unsigned int>(aValue); optValue) {
            return db_value { *optValue };
            }

         if (auto optValue = adecc::ConvertToOpt<unsigned long long>(aValue); optValue) {
            return db_value { *optValue };
            }

         if (auto optValue = adecc::ConvertToOpt<std::string>(aValue); optValue) {
            return db_value { *optValue };
            }

         return std::nullopt;
         }
      catch (...) {
         return std::nullopt;
         }
      }

   // ------------------------------------------------
   // Parameterbindung – nutzt ConvertFrom(...) → Variant
   // ------------------------------------------------
   framework_result_ty BindAllParams_(adecc::db_params const& params,
                                      ExceptionInformation const& call_info = { "Called from",
                                                                   src_loc::current(),
                                                                   std::chrono::system_clock::now() }) {
      if (!upQry) {
         return false;
         }

      try {
         for (auto const& p : params) {
            auto const& name = std::get<0>(p);
            auto const& val  = std::get<1>(p);
            bool const  req  = std::get<2>(p);

            TFDParam* prm = upQry->Params->FindParam(name.c_str());
            if (!prm) {
               if (!req) {
                  continue;
                  }

               return std::unexpected(error_ty { call_info.renew("Error at"), "Error while binding paramters",
                                                std::format("Parameter '{}' not found", name) });
               }

            std::visit(DbParamVariantWriter { prm }, val);
            }

         return true;
         }
      catch (Exception const& ex) {
         return std::unexpected(error_ty { {}, "Error while binding paramters.",
                                  ConvertTo<std::string>(ex.Message) });
         }
      }


   /*!
     \brief Visitor zum Binden eines einzelnen Variant-Parameters auf ein TFDParam
     \details
       - Nutzt die vorhandenen Convert<From, System::Variant>::apply(...) Spezialisierungen
       - Deckt alle Alternativen aus \c db_param (defined_param_types) ab
       - Schreibt konsistent in \c prm->Value
   */
   struct DbParamVariantWriter {
      TFDParam* prm{ nullptr };

      template <class ty>
      void operator()(std::optional<ty> const& optValue) const {
         using clean_ty = std::remove_cvref_t<ty>;

         prm->DataType = FireDacFieldType_<clean_ty>();

         if (!optValue) {
            prm->Clear();
            prm->Bound = true;
            return;
            }

         (*this)(*optValue);
         }

      // Strings
      void operator()(std::string_view const sv) const {
         prm->DataType = Data::Db::ftWideString;
         prm->Value = adecc::Convert<std::string_view, System::Variant>::apply(sv);
         prm->Bound = true;
         }

      void operator()(char const* const p) const {
         prm->DataType = Data::Db::ftWideString;
         prm->Value = adecc::Convert<char const*, System::Variant>::apply(p);
         prm->Bound = true;
         }

      void operator()(wchar_t const* const p) const {
         prm->DataType = Data::Db::ftWideString;
         prm->Value = adecc::Convert<System::UnicodeString, System::Variant>::apply(System::UnicodeString{ p });
         prm->Bound = true;
         }

      void operator()(std::string const& s) const {
         prm->DataType = Data::Db::ftWideString;
         prm->Value = adecc::Convert<std::string, System::Variant>::apply(s);
         prm->Bound = true;
         }

      // Zahlen / Bool
      void operator()(double const v) const {
         prm->DataType = Data::Db::ftFloat;
         prm->AsFloat = v;
         prm->Bound = true;
         }

      void operator()(money_ty const v) const {
         prm->DataType = Data::Db::ftFloat;
         prm->AsFloat = static_cast<double>(v);
         prm->Bound = true;
         }

      void operator()(int const v) const {
         prm->DataType = Data::Db::ftInteger;
         prm->AsInteger = v;
         prm->Bound = true;
         }

      void operator()(unsigned int const v) const {
         prm->DataType = Data::Db::ftInteger;
         prm->AsInteger = static_cast<int>(v);
         prm->Bound = true;
         }

      void operator()(long long const v) const {
         prm->DataType = Data::Db::ftLargeint;
         prm->AsLargeInt = static_cast<System::Int64>(v);
         prm->Bound = true;
         }

      void operator()(unsigned long long const v) const {
         prm->DataType = Data::Db::ftLargeint;
         prm->AsLargeInt = static_cast<System::Int64>(v);
         prm->Bound = true;
         }

      void operator()(bool const v) const {
         prm->DataType = Data::Db::ftBoolean;
         prm->AsBoolean = v;
         prm->Bound = true;
         }

      // Datum/Zeit (Chrono-Aliase in adecc)
      void operator()(adecc::date_ty const& v) const {
         prm->DataType = Data::Db::ftDate;
         prm->Value = adecc::Convert<adecc::date_ty, System::Variant>::apply(v);
         prm->Bound = true;
         }

      void operator()(adecc::timestamp_ty const& v) const {
         prm->DataType = Data::Db::ftDateTime;
         prm->Value = adecc::Convert<adecc::timestamp_ty, System::Variant>::apply(v);
         prm->Bound = true;
         }

      void operator()(adecc::time_ty const& v) const {
         prm->DataType = Data::Db::ftTime;
         prm->Value = adecc::Convert<adecc::time_ty, System::Variant>::apply(v);
         prm->Bound = true;
         }
   };



private:
   TFDConnection*                         pDbConn{ nullptr };
   std::unique_ptr<TFDQuery, query_del>   upQry{};
   std::string                            strOutputSql{};
   db_output_parameters                   vecOutputParameters{};
   bool                                   boHasIdentityOutput{ false };
   //bool                                   bStarted{ false };
};


template <class db_ty> using fw_query = query<db_ty>;  // erfüllt framework_query_type<fw_query, phys_db>



} // end of namespace adecc::db::delphi



