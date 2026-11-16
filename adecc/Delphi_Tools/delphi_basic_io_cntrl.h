#pragma once

#include "delphi_concepts.h"
#include "delphi_ui_auto_stream.h"
#include "delphi_waitcursor.h"
#include "delphi_form_support.h"

#include "stream_redirect.h"
#include "scope.h"

#include <ostream>
#include <string>
#include <memory>
#include <stdexcept>
#include <expected>


namespace adecc::delphi {

template <DelphiForm form_ty>
struct IOCntrl : public TFormGuard<form_ty, false> {

   template <StreamPolicy ty>
   struct AutoRedirect {
      using char_ty   = typename ty::char_type;
      using traits_ty = typename ty::traits_type;
      using buf_base  = typename ty::streambuf;
      using ostream   = typename ty::ostream;

      AutoStreamBuf<ty>                  aBuf;
      adecc::StreamRedirect<char_ty, traits_ty> aRedir;

      AutoRedirect(ostream& theStream,
                   System::Classes::TComponent *const pRoot,
                   std::string const & strName,
                   bool const boPreferLines,
                   bool const boClearLines)
                   : aBuf{pRoot, strName, boPreferLines, boClearLines}
                   , aRedir{theStream, static_cast<buf_base&>(aBuf)} { }
      };

   using redir_ty = AutoRedirect<AnsiStreamPolicy>;

   //form_ty* frm;  // auf jeden Fall geborgt, keine Freigabe!!
   mutable std::ostream time_stream {static_cast<std::streambuf*>(nullptr)};
   std::unique_ptr<redir_ty> my_cout;
   std::unique_ptr<redir_ty> my_cerr;
   std::unique_ptr<redir_ty> my_clog;
   std::unique_ptr<redir_ty> my_time;

   IOCntrl(form_ty* _frm, std::string const& strCout,  std::string const& strCerr = "",
           std::string const& strClog = "", std::string const& strTime = "") : TFormGuard<form_ty, false>(_frm) {
      
      if(strCout.size() == 0) throw std::invalid_argument("IOCntrl: Fehlendes Argument für std::cout redirection.");
      else                   my_cout = std::make_unique<redir_ty>(std::cout,   this->form, strCout, /*preferLines=*/true,  /*clearLines=*/true);
      if(strCerr.size() > 0) my_cerr = std::make_unique<redir_ty>(std::cerr,   this->form, strCerr, /*preferLines=*/true,  /*clearLines=*/true);
      if(strClog.size() > 0) my_clog = std::make_unique<redir_ty>(std::clog,   this->form, strClog, /*preferLines=*/false, /*clearLines=*/true);
      if(strTime.size() > 0) my_time = std::make_unique<redir_ty>(time_stream, this->form, strTime, /*preferLines=*/false, /*clearLines=*/true);
      }  

   ~IOCntrl() {
      my_cout.reset();
      my_cerr.reset();
      my_clog.reset();
      my_time.reset();
      }

   template <typename func_ty, typename... Args> requires std::invocable<func_ty, Args...>
   constexpr decltype(auto) invoke(func_ty && aFn, Args &&... aArgs) noexcept(std::is_nothrow_invocable_v<func_ty, Args...>) {
      using result_ty = std::invoke_result_t<func_ty, Args...>;
      my_cout->aBuf.ClearAll();
      my_cout->aBuf.BeginUpdate();
      //auto const theOnExit = std::scope_exit{ [&]() { my_cout->aBuf.EndUpdate(); } };    // not available for CB13 
      auto const theOnExit = adecc::makeScopeExit([&]() { my_cout->aBuf.EndUpdate(); });
      Application->ProcessMessages();
      if constexpr (std::is_void_v<result_ty>) {   // std::is_nothrow_invocable_v<func_ty, Args...>
         try {
            WaitCursor aWait{};  // RAII: set / restore Cursor
       
            auto const tStart = std::chrono::high_resolution_clock::now();

            std::invoke(std::forward<func_ty>(aFn), std::forward<Args>(aArgs)...);

            auto const tEnd  = std::chrono::high_resolution_clock::now();
            auto const us    = std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count();
            std::println(time_stream, "{:.3f} ms", us / 1000.0);
            }
         catch(Exception const& ex) {
            std::println(std::cerr, "Delphi Fehler: {}", AnsiString(ex.Message).c_str());
            }
         catch(std::exception const& ex) {
            std::println(std::cerr, "C++ Fehler: {}", ex.what());
            }
         }
      else {
         using expected_ty = std::expected<result_ty, std::string>;
         try {

            WaitCursor aWait {}; 
            auto const tStart = std::chrono::high_resolution_clock::now();
         
            result_ty ret = std::invoke(std::forward<func_ty>(aFn), std::forward<Args>(aArgs)...);

            auto const tEnd  = std::chrono::high_resolution_clock::now();
            auto const us    = std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count();
            std::println(time_stream, "{:.3f} ms", us / 1000.0);
            return expected_ty{std::in_place, std::move(ret)}; // success (value)
            }
         catch(Exception const& ex) {
            return expected_ty{std::unexpected(std::format("Delphi- Fehler: {}", AnsiString(ex.Message).c_str())) };
            }
         catch(std::exception const& ex) {
            return expected_ty{std::unexpected(std::format("Fehler: {}", ex.what())) };
            }
         }
      }
};

template <DelphiForm SelfT>
IOCntrl(SelfT*, std::string const&, std::string const&, std::string const&, std::string const&)
   -> IOCntrl<SelfT>;

} // end of namespace adecc::delphi

