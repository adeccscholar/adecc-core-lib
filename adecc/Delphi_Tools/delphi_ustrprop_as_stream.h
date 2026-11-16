#pragma once

#include "delphi_concepts.h"
#include "delphi_UI_core.h"
#include "stream_redirect.h"

#include <ostream>
#include <streambuf>
#include <string>
#include <string_view>
#include <type_traits>
#include <concepts>

namespace adecc::delphi {

/**
   \brief Streambuf für Text-/Caption-Eigenschaften (UnicodeString) von Delphi-Komponenten
   \details
      - Template-Parameter:
         - OwnerT: Typ des Besitzers mit schreibbarer UnicodeString-Property (Text oder Caption)
         - Ty: StreamPolicy (AnsiStreamPolicy oder WideStreamPolicy)
      - Semantik:
         - Zeichen werden im internen Stringstream gepuffert.
         - Beim Schreiben eines Newlines (cNL) wird der Pufferinhalt in die Property geschrieben
           (\e überschreiben, nicht anhängen) und der Puffer geleert.
      - Abhängigkeiten:
         - Keine direkten Includes von VCL oder FMX; nutzt nur RTTI-/System-Header aus den Basis-Dateien.
*/
template <HasWritableUnicodeStringProp owner_ty, StreamPolicy Ty = AnsiStreamPolicy>
class UstrPropStreamBuf : public StreamBufBase<Ty> {
private:
   owner_ty * pOwner{};

   static owner_ty * validateOwner(owner_ty *const p) {
      if(p == nullptr) {
         throw std::invalid_argument("UstrPropStreamBuf: pOwner must not be null");
      }
      return p;
   }

   /**
      \brief Überträgt den aktuellen Pufferinhalt in die UnicodeString-Property des Owners
      \pre pOwner != nullptr
      \post Property (Text/Caption) wurde auf den Pufferinhalt gesetzt
   */
   void setFromBuffer() {
      using char_type = typename Ty::char_type;

      typename Ty::stringstream & os = StreamBufBase<Ty>::os;
      auto const str = os.str();

      // Leere Schreibungen sind erlaubt: Property wird auf "" gesetzt
      if constexpr (std::is_same_v<char_type, char>) {
         // ANSI -> UnicodeString via AnsiString
         System::UnicodeString const u = System::UnicodeString(System::AnsiString(str.c_str()));
         set_ustr(pOwner, u);
      }
      else if constexpr (std::is_same_v<char_type, wchar_t>) {
         // Wide direkt nach UnicodeString
         System::UnicodeString const u(str.c_str());
         set_ustr(pOwner, u);
      }
      else {
         static_assert(std::is_same_v<char_type, char> || std::is_same_v<char_type, wchar_t>,
                       "UstrPropStreamBuf: unsupported char_type");
      }
   }

public:
   /**
      \brief Ctor
      \param pOwner_   Zeiger auf Owner mit Text/Caption
      \param boInitFromCurrent   Falls true, initialisiert der interne Puffer mit dem aktuellen Property-Wert,
                                 andernfalls wird mit leerem Puffer gestartet. Das Verhalten beeinflusst nur
                                 den ersten Write nach dem ersten '\n'.
   */
   explicit UstrPropStreamBuf(owner_ty *const pOwner_, bool const boInitFromCurrent = false)
               : StreamBufBase<Ty>{}, pOwner{validateOwner(pOwner_)} {
      if(boInitFromCurrent) {
         // Optional: Puffer mit vorhandenem Inhalt füllen (readable ist über HasUnicodeStringProp abgedeckt,
         // aber wir verlassen uns hier nur auf "writable" – defensive Abfrage:
         if constexpr (HasReadableUnicodeStringProp<owner_ty>) {
            System::UnicodeString const u = get_ustr(pOwner);
            if constexpr (std::is_same_v<typename Ty::char_type, char>) {
               StreamBufBase<Ty>::os << std::string(System::AnsiString(u).c_str());
               }
            else {
               StreamBufBase<Ty>::os << std::wstring(u.c_str());
               }
            }
         }
      }

   ~UstrPropStreamBuf(void) override {
      pOwner = nullptr;
   }

   /*!
      \brief Schreiben des Puffers in die Property (Überschreiben)
      \note Wird intern von overflow(cNL) aufgerufen.
   */
   void Write(void) override {
      setFromBuffer();
      // Puffer vollständig leeren
      StreamBufBase<Ty>::os.str(Ty::strEmpty);
      StreamBufBase<Ty>::os.clear();
      }
};

/*! \brief CTAD-Deduction-Guide für ANSI-Streams */
template <HasWritableUnicodeStringProp owner_ty>
UstrPropStreamBuf(owner_ty *, bool) -> UstrPropStreamBuf<owner_ty, AnsiStreamPolicy>;


/*!
   \brief Bequemer Ostream-Wrapper mit Besitz des Buffers (RAII)
   \details
      - Hält Buffer und std::basic_ostream zusammen.
      - Geeignet, wenn kein externer Stream umgebogen werden soll.
*/
template <HasWritableUnicodeStringProp owner_ty, StreamPolicy Ty = AnsiStreamPolicy>
class UstrPropOstream {
public:
   using char_type   = typename Ty::char_type;
   using traits_type = typename Ty::traits_type;
   using ostream_t   = std::basic_ostream<char_type, traits_type>;

   explicit UstrPropOstream(owner_ty *const pOwner, bool const boInitFromCurrent = false)
      : aBuf{pOwner, boInitFromCurrent}, os{nullptr} {
      os.rdbuf(static_cast<typename Ty::streambuf*>(&aBuf));
   }

   ~UstrPropOstream(void) {
      os.flush();
      os.rdbuf(nullptr);
   }

   UstrPropOstream(UstrPropOstream const &) = delete;
   UstrPropOstream & operator=(UstrPropOstream const &) = delete;

   UstrPropOstream(UstrPropOstream &&) = default;
   UstrPropOstream & operator=(UstrPropOstream &&) = default;

   ostream_t & stream() { return os; }
   operator ostream_t &() { return os; }

private:
   UstrPropStreamBuf<owner_ty, Ty> aBuf;
   ostream_t os;
};

} // end of namespace adecc::delphi

