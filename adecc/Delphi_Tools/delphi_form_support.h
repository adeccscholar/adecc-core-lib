#pragma once

#include "delphi_concepts.h"
#include "delphi_memo_as_stream.h"
#include "delphi_memo_as_range.h"

#include <System.UITypes.hpp>

#include <System.Rtti.hpp>
#include <System.TypInfo.hpp>
#include <System.SysUtils.hpp>

#include <memory>

#include <type_traits>
#include <concepts>

namespace adecc::delphi {

template <DelphiForm form_ty, bool Owning = true>
class TFormGuard {
   protected:       
      using handle_ty = std::conditional_t<(Owning == true), std::unique_ptr<form_ty>, form_ty* const>;
      handle_ty form;

      form_ty* raw() const noexcept  {
         if constexpr (Owning) { return form.get(); }
         else { return form; }
         }

      [[nodiscard]] System::Classes::TComponent *const FindComponent(std::string const& strName) const {
            System::Classes::TComponent *const pComp = form->FindComponent(strName.c_str());
            if(pComp == nullptr) throw std::runtime_error("TFormGuard::FindComponent: form has no component: " + strName);
            return pComp;
            }

      [[nodiscard]] System::Classes::TStrings* GetStringList(std::string const& strCompName,
                                                             std::string const& strPropName = "Lines") const {
         auto *const pForm = raw(); // durch Deine RAII-Konstruktoren garantiert != nullptr

         // 1) Komponente per Name finden
         System::UnicodeString const uCompName = System::UnicodeString(System::AnsiString(strCompName.c_str()));
         System::Classes::TComponent *const pComp = form->FindComponent(uCompName);
         if(pComp == nullptr) {
            throw std::runtime_error("TFormGuard::GetStringList: component not found: " + strCompName);
            }

         // 2) RTTI-Typ der Komponente holen
         System::Rtti::TRttiContext ctx;
         System::Rtti::TRttiType *const pType = ctx.GetType(pComp->ClassType());
         if(pType == nullptr) {
            throw std::runtime_error("TFormGuard::GetStringList: no RTTI for component: " + strCompName);
            }

         // 3) Property "Lines" (oder übergebenen Namen) ermitteln
         System::UnicodeString const uPropName = System::UnicodeString(System::AnsiString(strPropName.c_str()));
         System::Rtti::TRttiProperty *const pProp = pType->GetProperty(uPropName);
         if(pProp == nullptr || !pProp->IsReadable) {
            throw std::runtime_error("TFormGuard::GetStringList: component has no readable '" + strPropName + "': " + strCompName);
            }

         // 4) Wert lesen und in TStrings* casten
         System::Rtti::TValue v = pProp->GetValue(pComp);
         if(!v.IsObject()) {
            throw std::runtime_error("TFormGuard::GetStringList: property is not an object: " + strPropName);
            }
         auto *const pStrings = dynamic_cast<System::Classes::TStrings*>(v.AsObject());
         if(pStrings == nullptr) {
            throw std::runtime_error("TFormGuard::GetStringList: property is not TStrings*: " + strPropName);
            }

         return pStrings;
         }
   

   public:
      TFormGuard() = delete;

      template <typename ...Args> requires (Owning == true)
      TFormGuard(Args&&... args) {
         form = std::make_unique<form_ty>(std::forward<Args>(args)...);
         if(!form) throw std::runtime_error("TFormGuard: allocation failed");
         }

      template <typename param_form> 
           requires ((Owning == false) && std::is_base_of_v<form_ty, std::remove_cvref_t<param_form>>)
      TFormGuard(param_form* param) : form { param } {
         if (param == nullptr) throw std::invalid_argument("TFormGuard: param must not be null");
         }

      // Verhindert versehentliche Konstruktion aus nullptr
      template <bool O = Owning> requires (O == false)
      TFormGuard(std::nullptr_t) = delete;

      TFormGuard(TFormGuard const&) = delete;
      TFormGuard operator=(TFormGuard const&) = delete;

      template <bool O = Owning> requires (O == true)
      TFormGuard(TFormGuard&& other) noexcept : form(std::move(other.form)) {
         }

      template <bool O = Owning> requires (O == true)
      TFormGuard& operator=(TFormGuard&& other) noexcept {
         if (this != &other) {
            form = std::move(other.form);
            }
         return *this;
         }

      operator form_ty* () {
         if constexpr (Owning) { return form.get(); }
         else { return form; }
         } 

      operator form_ty* const () const {
         if constexpr (Owning) { return form.get(); }
         else { return form; }
         } 

      template <bool O = Owning> requires (O == false)
      TFormGuard(TFormGuard&& other) noexcept : form(other.form) {
         }

      // Rvalue-Zuweisung: verboten, weil der Member-Zeiger const ist.
      template <bool O = Owning> requires (O == false)
      TFormGuard& operator=(TFormGuard&&) = delete;

      void SetCaption(std::string const& msg) {
         form->Caption = msg.c_str(); 
         }  

      void Show() {
	     form->ShowModal();
	     }

      [[nodiscard]] RttiOstream OpenLinesStream(std::string const & strName,
                                                bool const boClearLines = false) const {
         return RttiOstream { raw(), strName, boClearLines };
         }

      [[nodiscard]] LinesView<LinesOwnerShim> OpenLinesView(std::string const & strName) const {
         System::Classes::TStrings *const pLines = GetStringList(strName, "Lines");

         // Adapter-Owner erstellen (nur für den Ctor; LinesView speichert danach nur TStrings*)
         LinesOwnerShim ownerShim{};
         ownerShim.Lines = pLines;

         return LinesView<LinesOwnerShim>{ &ownerShim };
         }

};
      

template <DelphiForm FormT>
class FormAsOstreams {
public:

   template <typename... Args> requires (std::constructible_from<FormT, Args...>)
   explicit FormAsOstreams(std::vector<std::string> vecNames, Args&&... aArgs)
                   : theForm{std::make_unique<FormT>(std::forward<Args>(aArgs)...)} {

      for(std::string const& strName : vecNames) {
         System::Classes::TComponent *const pComp = findByName(theForm.get(), strName);
         if(pComp == nullptr) {
            throw std::runtime_error("FormAsOstreams: component not found: " + strName);
            }
         System::Classes::TStrings *const pLines = tryGetStringList(pComp, "Lines");
         if(pLines == nullptr) {
            throw std::runtime_error("FormAsOstreams: component has no readable Lines: " + strName);
            }

         // Owner + Streambuf (ANSI 1:1) anlegen
         auto upOwner = std::make_unique<LinesOwnerShim>();
         upOwner->Lines = pLines;

		 using BufT = LinesStreamBuf<LinesOwnerShim, AnsiStreamPolicy>;
		 auto upBuf = std::make_unique<BufT>(upOwner.get(), true);

		 // std::ostream erstellen und rdbuf setzen
		 auto upOs = std::make_unique<std::ostream>(nullptr);
		 upOs->rdbuf(static_cast<AnsiStreamPolicy::streambuf*>(upBuf.get()));

		 // Besitz übernehmen
		 vecOwners.emplace_back(std::move(upOwner));
		 vecBufs.emplace_back(std::move(upBuf));
		 vecStreams.emplace_back(std::move(upOs));
	     }
      }

   ~FormAsOstreams(void) {
	  for(auto & upOs : vecStreams) {
		 if(upOs) {
			upOs->flush();
			}
		 }
	  vecStreams.clear();
	  vecBufs.clear();
	  vecOwners.clear();
	  theForm.reset();
	  }

   FormAsOstreams(FormAsOstreams const &) = delete;
   FormAsOstreams & operator=(FormAsOstreams const &) = delete;
   FormAsOstreams(FormAsOstreams &&) = delete;
   FormAsOstreams & operator=(FormAsOstreams &&) = delete;


   std::size_t Size() const {
	  return vecStreams.size();
	  }


   std::ostream & get(std::size_t const i) {
      if(i >= vecStreams.size()) {
         throw std::out_of_range("FormAsOstreams::get index out of range");
		 }
      return *vecStreams[i];
	  }

   FormT* Form() const {
	  return theForm.get();
	  }

   void SetCaption(std::string const& strText) {
	  theForm->Caption = strText.c_str();
	  }

   void Show() {
	  theForm->ShowModal();
	  }

private:
   std::unique_ptr<FormT> theForm{};
   std::vector<std::unique_ptr<LinesOwnerShim>> vecOwners{};
   std::vector<std::unique_ptr<LinesStreamBuf<LinesOwnerShim, AnsiStreamPolicy>>> vecBufs{};
   std::vector<std::unique_ptr<std::ostream>> vecStreams{};
};

} // end of namespace adecc::delphi

