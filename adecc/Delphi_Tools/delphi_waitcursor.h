#pragma once

#include <utility>
#include <type_traits>

namespace adecc {
/*!
   \brief Policy-Interface für Cursor-Umsetzung
   \details \note Jede Policy muss Init() und Done() bereitstellen.
*/
struct ICursorPolicy {
   void Init() {}
   void Done() {}
};


// -------------------------------------------------------------
// FMX-Policy (nur wenn FMX-Header vorhanden sind)
// -------------------------------------------------------------
#if __has_include(<FMX.Platform.hpp>) && __has_include(<FMX.Types.hpp>) && BUILD_WITH_FMX
  #include <FMX.Platform.hpp>
  #include <FMX.Types.hpp>

namespace delphi::fmx {

struct CursorPolicy : ICursorPolicy {
   using TCursor = ::System::Uitypes::TCursor;

   CursorPolicy(void) = default;
   ~CursorPolicy(void) = default;

   void Init() {
      using namespace Fmx;
      using ::System::DelphiInterface;

      if(::Fmx::Platform::TPlatformServices::Current &&
         ::Fmx::Platform::TPlatformServices::Current->SupportsPlatformService(
             __uuidof(::Fmx::Types::IFMXCursorService), (void*)&m_cs)) {
         m_old = m_cs->GetCursor();
         m_cs->SetCursor(::System::Uitypes::crHourGlass);
      }
   }

   void Done() {
      if(m_cs) {
         m_cs->SetCursor(m_old);
      }
      m_cs = nullptr;
   }

private:
   ::Fmx::Types::_di_IFMXCursorService m_cs{};
   TCursor m_old{::System::Uitypes::crDefault};
};

} // end of namespace delphi::fmx

#endif // ADECC_HAS_FMX


// -------------------------------------------------------------
// VCL-Policy (nur wenn VCL-Header vorhanden sind)
// -------------------------------------------------------------
#if __has_include(<Vcl.Forms.hpp>) && __has_include(<Vcl.Graphics.hpp>) && BUILD_WITH_VCL
  #include <Vcl.Forms.hpp>
  #include <Vcl.Graphics.hpp>

namespace delphi::vcl {

struct CursorPolicy : ICursorPolicy {
   using TCursor = ::System::Uitypes::TCursor;

   void Init() {
      m_old = ::Vcl::Forms::Screen->Cursor;
      ::Vcl::Forms::Screen->Cursor = ::System::Uitypes::crHourGlass;
   }

   void Done() {
      ::Vcl::Forms::Screen->Cursor = m_old;
   }

private:
   TCursor m_old{::System::Uitypes::crDefault};
};

} // end of namespace delphi::vcl

#endif // ADECC_HAS_VCL


// -------------------------------------------------------------
// WinAPI-Fallback (nur Windows, wenn weder FMX noch VCL gewählt)
// -------------------------------------------------------------
#if !defined(BUILD_WITH_FMX) && !defined(BUILD_WITH_VCL) && defined(_WIN32)
  #define HAS_WIN_FALLBACK
  #include <windows.h>

struct WinCursorPolicy : ICursorPolicy {
   void Init() {
      m_old = ::GetCursor();
      ::SetCursor(::LoadCursorW(nullptr, IDC_WAIT));
   }
   void Done() {
      if(m_old) {
         ::SetCursor(m_old);
      }
   }
private:
   HCURSOR m_old{};
};

#endif // ADECC_HAS_WIN_FALLBACK


// -------------------------------------------------------------
// No-Op-Policy (alle anderen Plattformen / Toolchains)
// -------------------------------------------------------------
#if !defined(BUILD_WITH_FMX) && !defined(BUILD_WITH_VCL) && !defined(HAS_WIN_FALLBACK)

struct NoopCursorPolicy : ICursorPolicy {
   void Init() { }
   void Done() { }
};

#endif


// -------------------------------------------------------------
// Policy-Selector
// -------------------------------------------------------------
namespace detail {

#if defined(BUILD_WITH_FMX)
   using DefaultCursorPolicy = delphi::fmx::CursorPolicy;
#elif defined(BUILD_WITH_VCL)
   using DefaultCursorPolicy = delphi::vcl::CursorPolicy;
#elif defined(HAS_WIN_FALLBACK)
   using DefaultCursorPolicy = WinCursorPolicy;
#else
   using DefaultCursorPolicy = NoopCursorPolicy;
#endif

} // namespace detail


/*!
   \brief RAII-Wartecursor ohne harte FMX/VCL-Abhängigkeit
   \details
      \tparam PolicyT Cursor-Policy (per Default auto-gewählt über Header-Verfügbarkeit)
   \note
      \pre PolicyT stellt Init() und Done() bereit
      \post Destruktor stellt den vorherigen Cursor-Status sicher wieder her
*/
template <typename PolicyT = detail::DefaultCursorPolicy>
class WaitCursor {
public:
   WaitCursor(void) { m_policy.Init(); }

   WaitCursor(WaitCursor const&) = delete;
   WaitCursor & operator=(WaitCursor const&) = delete;

   WaitCursor(WaitCursor && other) noexcept
      : m_policy(std::move(other.m_policy)), m_active{other.m_active} {
      other.m_active = false;
      }

   WaitCursor& operator=(WaitCursor && other) noexcept {
      if(this != &other) {
         if(m_active) { m_policy.Done(); }
         m_policy = std::move(other.m_policy);
         m_active = other.m_active;
         other.m_active = false;
      }
      return *this;
   }

   ~WaitCursor(void) {
      if(m_active) { m_policy.Done(); }
      m_active = false;
   }

   /*!
      \brief Vorzeitiges Zurücksetzen
      \post Cursor-Status ist wiederhergestellt, Objekt ist inert
   */
   void Reset() {
      if(m_active) {
         m_policy.Done();
         m_active = false;
      }
   }

private:
   PolicyT m_policy{};
   bool m_active{true};
};


} // end of namespace adecc

