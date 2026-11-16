#pragma once

#include "delphi_concepts.h"
#include "delphi_UI_core.h"
#include "delphi_memo_as_stream.h"

#include "stream_redirect.h"

#include <ostream>
#include <streambuf>
#include <string>
#include <string_view>
#include <variant>
#include <stdexcept>
#include <utility>
#include <type_traits>
#include <concepts>

namespace adecc::delphi {

/*!
   \brief Typ-erased Setter für UnicodeString-Properties (Text/Caption)
   \details
      \note Umgeht Template-Abhängigkeiten; speichert nur Funktionszeiger + Objektzeiger.
*/
struct TextSetter {
   using FnT = void(*)(void*, System::UnicodeString const&);
   void * pObj{};
   FnT fn{};

   template <typename owner_ty> requires HasWritableUnicodeStringProp<owner_ty>
   static TextSetter Make(owner_ty *const pOwner) {
      return TextSetter{
         pOwner,
         +[](void * p, System::UnicodeString const & u) {
            set_ustr(static_cast<owner_ty*>(p), u);
         }
      };
   }

   static TextSetter FromStringProperty(StringPropertyRef const & aRef) {
      // Achtung: StringPropertyRef arbeitet mit ANSI round-trip; für UI reicht das idR.
      return TextSetter{
         // pObj zeigt hier auf eine kopierte Ref-Instanz im Heap
         new StringPropertyRef(aRef), [](void * p, System::UnicodeString const & u) {
                                           auto * pRef = static_cast<StringPropertyRef*>(p);
                                           System::AnsiString const a(u);
                                           pRef->Set(std::string(a.c_str()));
         }
      };
   }

   void Set(System::UnicodeString const & u) const {
      if(fn) fn(pObj, u);
   }

   explicit operator bool() const { return fn != nullptr; }

   void Reset() {
      // Nur für den RTTI-Fall zu bereinigen
      if(pObj && fn == nullptr) { /* unreachable */ }
   }

   ~TextSetter() = default;
};

/*!
   \brief Vereinheitlichtes Streambuf: schreibt wahlweise in Lines oder Text/Caption
   \details
      - Newline ('\n' bzw. L'\n') triggert Write() und leert den Buffer
      - Keine direkten VCL/FMX-Includes; nutzt nur vorhandene RTTI/Helper
*/
template <StreamPolicy Ty = AnsiStreamPolicy>
class AutoStreamBuf : public StreamBufBase<Ty> {
private:
   enum class Mode { None, Lines, Text };

   struct LinesTarget {
      System::Classes::TStrings * pLines{};
      bool boClearAtStart{};
      };

   struct TextTarget {
      TextSetter theSetter{};
      };

   Mode eMode{Mode::None};
   std::variant<std::monostate, LinesTarget, TextTarget> varTarget{};

   static System::UnicodeString toUString(typename Ty::stringstream & os) {
      using char_type = typename Ty::char_type;
      auto const str = os.str();
      if constexpr (std::is_same_v<char_type, char>) {
         return System::UnicodeString(System::AnsiString(str.c_str()));
         } 
      else {
         return System::UnicodeString(str.c_str());
         }
      }

   void ctorInitLines(System::Classes::TStrings *const pLines, bool const boClear) {
      if(pLines == nullptr) {
         throw std::invalid_argument("AutoStreamBuf: Lines pointer must not be null");
         }
      eMode = Mode::Lines;
      varTarget = LinesTarget{pLines, boClear};
      if(boClear) { pLines->Clear(); }
      }

   void ctorInitText(TextSetter const & set) {
      if(!set) {
         throw std::invalid_argument("AutoStreamBuf: TextSetter must be valid");
         }
      eMode = Mode::Text;
      varTarget = TextTarget{set};
      }

public:
   /*!
      \brief Compile-Time: OwnerT* -> Auswahl per Concepts
      \param pOwner          konkreter Owner
      \param boClearLines    Lines beim Start leeren (nur relevant, wenn Lines-Zweig aktiv)
   */
   template <typename owner_ty>
      requires (HasLinesOrItems<owner_ty> || HasWritableUnicodeStringProp<owner_ty>)
   explicit AutoStreamBuf(owner_ty *const pOwner, bool const boClearLines = false) : StreamBufBase<Ty>{} {
      if(pOwner == nullptr) {
         throw std::invalid_argument("AutoStreamBuf: pOwner must not be null");
         }
      if constexpr (HasLinesOrItems<owner_ty>) {
         ctorInitLines(pOwner->Lines, boClearLines);
         } 
      else if constexpr (HasWritableUnicodeStringProp<owner_ty>) {
         ctorInitText(TextSetter::Make(pOwner));
         } 
      else {
         static_assert(HasLinesOrItems<owner_ty> || HasWritableUnicodeStringProp<owner_ty>,
                       "OwnerT must provide Lines or writable Text/Caption");
         }
      }

   /*!
      \brief RTTI: Form + Komponentenname -> automatische Auswahl
      \param pRoot           Formular/Owner
      \param strName         Komponentenname
      \param boPreferLines   true: Lines bevorzugen, wenn vorhanden
      \param boClearLines    Lines beim Start leeren (nur wenn Lines gewählt wurde)
   */
   explicit AutoStreamBuf(System::Classes::TComponent *const pRoot,
                          std::string const & strName,
                          bool const boPreferLines = true,
                          bool const boClearLines = false)
      : StreamBufBase<Ty>{} {
      System::Classes::TComponent *const pComp = findByName(pRoot, strName);
      if(pComp == nullptr) {
         throw std::runtime_error("AutoStreamBuf: component not found: " + strName);
      }

      System::Classes::TStrings *const pLines = tryGetStringList(pComp, "Lines");
      auto const optText = TryGetStringProperty(pComp, "Text");
      auto const optCaption = TryGetStringProperty(pComp, "Caption");
      auto const optSimpleText = TryGetStringProperty(pComp, "SimpleText");
      // Auswahlreihenfolge
      if(boPreferLines && pLines != nullptr) {
         ctorInitLines(pLines, boClearLines);
         }
      else if(optText && optText->IsWritable()) {
         ctorInitText(TextSetter::FromStringProperty(*optText));
         }
      else if(optCaption && optCaption->IsWritable()) {
         ctorInitText(TextSetter::FromStringProperty(*optCaption));
         }
      else if(optSimpleText && optSimpleText->IsWritable()) {
         ctorInitText(TextSetter::FromStringProperty(*optSimpleText));
         }
      else if(pLines != nullptr) {
         ctorInitLines(pLines, boClearLines);
         }
      else {
         throw std::runtime_error("AutoStreamBuf: component has neither writable Text/Caption nor Lines");
      }
   }

   ~AutoStreamBuf(void) override = default;

   /*!
      \brief Newline-Commit
      \post Puffer geleert
   */
   void Write(void) override {
      switch(eMode) {
         case Mode::Lines: {
            auto & a = std::get<LinesTarget>(varTarget);
            auto const u = toUString(StreamBufBase<Ty>::os);
            if(u.Length() > 0) {
               a.pLines->Add(u);
               } 
            else {
               a.pLines->Add(L"");
               }
            break;
            }
         case Mode::Text: {
            auto & t = std::get<TextTarget>(varTarget);
            t.theSetter.Set(toUString(StreamBufBase<Ty>::os));
            break;
            }
         default:
            break;
         }
      StreamBufBase<Ty>::os.str(Ty::strEmpty);
      StreamBufBase<Ty>::os.clear();
      }

   /*!
      \brief Löscht den aktuellen Buffer-Inhalt ohne zu schreiben
      \details Entfernt alle bislang gepufferten Zeichen. Weder Lines noch Text/Caption
               werden verändert. Statusbits des internen Streams werden zurückgesetzt.
      \post Puffer ist leer.
   */
   void Clear(void) {
      StreamBufBase<Ty>::os.str(Ty::strEmpty);
      StreamBufBase<Ty>::os.clear();
      }


   void BeginUpdate() {
      switch(eMode) {
         case Mode::Lines: {
            auto & a = std::get<LinesTarget>(varTarget);
            if(a.pLines != nullptr) {
               a.pLines->BeginUpdate();      
               }
            break;
            }
         default:
            break;
         }
      }
             
   void EndUpdate() {
      switch(eMode) {
         case Mode::Lines: {
            auto & a = std::get<LinesTarget>(varTarget);
            if(a.pLines != nullptr) {
               a.pLines->EndUpdate();      
               }
            break;
            }
         default:
            break;         }
      }
             

   /*!
      \brief Leert die Ausgabe selbst
      \details
         - Mode::Lines  : ruft Lines->Clear() auf
         - Mode::Text   : setzt Text/Caption auf ""
      \post Ziel ist geleert. Puffer bleibt unverändert.
      \throw Weitergabe von Ausnahmen aus TStrings::Clear oder Property-Setter
   */
   void ClearOutput(void) {
      switch(eMode) {
         case Mode::Lines: {
            auto & a = std::get<LinesTarget>(varTarget);
            if(a.pLines != nullptr) {
               a.pLines->BeginUpdate();
               try {
                  a.pLines->Clear();
                  } 
               catch(...) {
                  a.pLines->EndUpdate();
                  throw;
                  }
               a.pLines->EndUpdate();
               }
            break;
            }
         case Mode::Text: {
            auto & t = std::get<TextTarget>(varTarget);
            t.theSetter.Set(System::UnicodeString{}); // leeres UnicodeString
            break;
            }
         default:
            break;
         }
      }

   /*!
      \brief Leert Puffer und Ausgabe
      \post Ziel ist leer und Puffer ist leer.
   */
   void ClearAll(void) {
      ClearOutput();
      Clear();
      }
};

/*!
   \brief Komfortabel: hält Buffer + ostream zusammen (RAII)
   \details
      \note Für ANSI -> std::ostream, für Wide -> std::wostream
*/
template <StreamPolicy Ty = AnsiStreamPolicy>
class AutoOstream {
public:
   using char_type   = typename Ty::char_type;
   using traits_type = typename Ty::traits_type;
   using ostream_t   = std::basic_ostream<char_type, traits_type>;

   template <typename owner_ty>
      requires (HasLinesOrItems<owner_ty> || HasWritableUnicodeStringProp<owner_ty>)
   explicit AutoOstream(owner_ty *const pOwner, bool const boClearLines = false)
      : aBuf{pOwner, boClearLines}, os{nullptr} {
      os.rdbuf(static_cast<typename Ty::streambuf*>(&aBuf));
   }

   explicit AutoOstream(System::Classes::TComponent *const pRoot,
                        std::string const & strName,
                        bool const boPreferLines = true,
                        bool const boClearLines = false)
      : aBuf{pRoot, strName, boPreferLines, boClearLines}, os{nullptr} {
      os.rdbuf(static_cast<typename Ty::streambuf*>(&aBuf));
   }

   ~AutoOstream(void) {
      os.flush();
      os.rdbuf(nullptr);
   }

   AutoOstream(AutoOstream const &) = delete;
   AutoOstream & operator=(AutoOstream const &) = delete;
   AutoOstream(AutoOstream &&) = default;
   AutoOstream & operator=(AutoOstream &&) = default;

   ostream_t & stream() { return os; }
   operator ostream_t &() { return os; }

private:
   AutoStreamBuf<Ty> aBuf;
   ostream_t os;
};

/*!
   \brief Bequemer Fabrik-Helper (ANSI)
*/
inline AutoOstream<AnsiStreamPolicy>
MakeAutoOstream(System::Classes::TComponent *const pRoot,
                std::string const & strName,
                bool const boPreferLines = true,
                bool const boClearLines = false) {
   return AutoOstream<AnsiStreamPolicy>{pRoot, strName, boPreferLines, boClearLines};
}

/*!
   \brief Bequemer Fabrik-Helper (WIDE)
*/
inline AutoOstream<WideStreamPolicy>
MakeAutoWOstream(System::Classes::TComponent *const pRoot,
                 std::string const & strName,
                 bool const boPreferLines = true,
                 bool const boClearLines = false) {
   return AutoOstream<WideStreamPolicy>{pRoot, strName, boPreferLines, boClearLines};
}

} // end of namespace adecc::delphi

