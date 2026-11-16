#pragma once

#include "delphi_concepts.h"
#include "delphi_convert.h"
#include "delphi_UI_core.h"
#include "stream_redirect.h"

#include <System.Rtti.hpp>
#include <System.TypInfo.hpp>
#include <System.SysUtils.hpp>

#include <optional>
#include <stdexcept>
#include <memory>
#include <vector>
#include <tuple>
#include <concepts>
#include <type_traits>

namespace adecc::delphi {

template <HasLinesOrItems owner_ty, StreamPolicy Ty = AnsiStreamPolicy>
class LinesStreamBuf : public StreamBufBase<Ty> {
private:
   owner_ty * pOwner {};

   static System::Classes::TStrings* ValidateAndGetSequence(owner_ty *const p) {
      if(p == nullptr) {                    
         throw std::invalid_argument("LinesStreamBuf: pOwner must not be null");
		 }
	  if(p->Lines == nullptr) {
         throw std::invalid_argument("LinesStreamBuf: owner->Lines must not be null");
		 }
      return p->Lines;
	  }

public:
   explicit LinesStreamBuf(owner_ty *const pOwner_, bool const boClean = true) : StreamBufBase<Ty>{}, pOwner{pOwner_} {
	  System::Classes::TStrings *const a = ValidateAndGetSequence(pOwner);
	  if(boClean) {
         a->Clear();
		 }
	  }

   ~LinesStreamBuf(void) override {
	  pOwner = nullptr;
	  }

   void Write(void) override {
	  if(StreamBufBase<Ty>::os.str().length() > 0) {
		 pOwner->Lines->Add(StreamBufBase<Ty>::os.str().c_str());
		 }
	  else {
		 pOwner->Lines->Add(L"");
		 }
	  }
};


template <HasLinesOrItems owner_ty>
LinesStreamBuf(owner_ty *, bool) -> LinesStreamBuf<owner_ty, AnsiStreamPolicy>;



class RttiOstream {
public:
   explicit RttiOstream(System::Classes::TComponent *const pRoot,
						std::string const & strName,
						bool const boClearLines = false) {
	  System::Classes::TComponent *const pComp = findByName(pRoot, strName);
	  if(pComp == nullptr) {
		 throw std::runtime_error("RttiOstream: component not found: " + strName);
		 }

	  System::Classes::TStrings *const pLines = tryGetStringList(pComp, "Lines");
	  if(pLines == nullptr) {
		 throw std::runtime_error("RttiOstream: component has no readable Lines: " + strName);
		 }

	  upOwner = std::make_unique<LinesOwnerShim>();
      upOwner->Lines = pLines;

	  using BufT = LinesStreamBuf<LinesOwnerShim, AnsiStreamPolicy>;
      upBuf = std::make_unique<BufT>(upOwner.get(), boClearLines);

	  upOs = std::make_unique<std::ostream>(nullptr);
      upOs->rdbuf(static_cast<AnsiStreamPolicy::streambuf*>(upBuf.get()));
   }

   ~RttiOstream(void) {
      if(upOs) {
         upOs->flush();
		 }
      upOs.reset();
      upBuf.reset();
      upOwner.reset();
	  }

   RttiOstream(RttiOstream const &) = delete;
   RttiOstream & operator=(RttiOstream const &) = delete;
   RttiOstream(RttiOstream &&) = delete;
   RttiOstream & operator=(RttiOstream &&) = delete;

   std::ostream& stream() {
      return *upOs;
	  }

   operator std::ostream &() {
	  return *upOs;
	  }


private:
   std::unique_ptr<LinesOwnerShim> upOwner{};
   std::unique_ptr<LinesStreamBuf<LinesOwnerShim, AnsiStreamPolicy>> upBuf{};
   std::unique_ptr<std::ostream> upOs{};
};


inline RttiOstream MakeRttiOstream(System::Classes::TComponent *const pRoot,
                                   std::string const & strName,
                                   bool const boClearLines = false) {
   return RttiOstream{pRoot, strName, boClearLines};
}



/*
class StringPropertyRef {
public:
   StringPropertyRef(System::Classes::TComponent *const pComp,
					 System::Rtti::TRttiProperty *const pProp)
   : pComp{pComp}, pProp{pProp} { }

   std::string Get() const {
	  EnsureValidReadable();
	  System::Rtti::TValue v = pProp->GetValue(pComp);
	  System::UnicodeString const theUni = v.AsString();
	  System::AnsiString const theAnsi = System::AnsiString(theUni);
	  return std::string(theAnsi.c_str());
	  }

   template <typename ty>
   void Set(ty const & val) const {
	  EnsureValidWritable();
	  System::UnicodeString const theUni = adecc::ConvertTo<System::UnicodeString>(val);
	  pProp->SetValue(pComp, System::Rtti::TValue::From<System::UnicodeString>(theUni));
	  }

   operator std::string() const {
	  return Get();
	  }

   StringPropertyRef & operator=(std::string const & strValue) {
	  Set(strValue);
	  return *this;
	  }

   bool IsWritable() const {
	  return pProp && pProp->IsWritable;
	  }

   bool IsReadable() const {
	  return pProp && pProp->IsReadable;
	  }


   bool IsValid() const {
	  return pComp != nullptr && pProp != nullptr;
	  }

private:
   void EnsureValidReadable() const {
	  if(!(IsValid() && IsReadable())) {
		 throw std::runtime_error("StringPropertyRef: property not readable");
		 }
	  }

   void EnsureValidWritable() const {
	  if(!(IsValid() && IsWritable())) {
		 throw std::runtime_error("StringPropertyRef: property not writable");
		 }
	  }

   System::Classes::TComponent *pComp{};
   System::Rtti::TRttiProperty *pProp{};
};
*/

 class StringPropertyRef {
 public:
    // Neuer Ctor: wir speichern nur Component + Property-Namen
    StringPropertyRef(System::Classes::TComponent *const pComp_,
                      System::UnicodeString const & uPropName_)
    : pComp{pComp_}, uPropName{uPropName_} { }

    std::string Get() const {
       auto * p = resolve();
       if(!(p && p->IsReadable)) {
          throw std::runtime_error("StringPropertyRef: property not readable");
       }
       System::Rtti::TValue v = p->GetValue(pComp);
       System::UnicodeString const theUni = v.AsString();
       System::AnsiString   const theAnsi = System::AnsiString(theUni);
       return std::string(theAnsi.c_str());
       }

    
    void Set(std::string const & strValue) const {
       /*
       auto * p = resolve();
       if(!(p && p->IsWritable)) {
          throw std::runtime_error("StringPropertyRef: property not writable");
          }
       System::UnicodeString const theUni = adecc::ConvertTo<System::UnicodeString>(aValue);
       p->SetValue(pComp, System::Rtti::TValue::From<System::UnicodeString>(theUni));
       }
       */
      using namespace System;
      using namespace System::Rtti;
      using namespace System::Typinfo;

      TRttiProperty * p = resolve();
      if(!(p && p->IsWritable)) {
         throw std::runtime_error("StringPropertyRef: property not writable");
         }

      // Eingabe sauber nach UnicodeString heben (UTF-8 ? UTF-16, NICHT ANSI-roundtrip)
      // Annahme: std::string ist UTF-8. Wenn bei dir anders: hier zentral anpassen.
      UnicodeString const uVal = adecc::ConvertTo<UnicodeString>(strValue);

      // 1) Typgenaues Setzen über RTTI
      TRttiType * pPropType = p->PropertyType;
      if(pPropType && isStringLikeKind(pPropType->TypeKind)) {
         try {
            TValue tv;
            // Für Unicode-Strings reicht das:
            tv = TValue::From<UnicodeString>(uVal);
            p->SetValue(pComp, tv);
            return;
         } catch(...) {
            // Fallback unten
         }
      }

      // 2) Fallback: TypInfo-Weg (sehr tolerant gegenüber Alias-Typen wie TCaption)
      try {
         setViaTypInfo(pComp, uPropName, uVal);
         return;
      } catch(...) {
         // 3) Letzter Versuch: Konservative Konvertierung über WideString
         try {
            TValue tv = TValue::From<WideString>(WideString(uVal));
            p->SetValue(pComp, tv);
            return;
         } catch(...) {
            // Diagnose erleichtern
            UnicodeString uClass = pComp ? pComp->ClassName() : L"<null>";
            UnicodeString uType  = pPropType ? pPropType->Name : L"<null>";
            UnicodeString uMsg   = L"StringPropertyRef::Set failed for "
                                    + uClass + L"." + uPropName
                                    + L" of type " + uType;
            throw std::runtime_error(AnsiString(uMsg).c_str());
         }
      }
       }

    operator std::string() const { return Get(); }

    StringPropertyRef & operator=(std::string const & strValue) {
       Set(strValue);
       return *this;
       }

    bool IsWritable() const {
       auto * p = resolve();
       return p && p->IsWritable;
       }

    bool IsReadable() const {
       auto * p = resolve();
       return p && p->IsReadable;
       }

    bool IsValid() const {
       return pComp != nullptr && resolve() != nullptr;
    }

 private:
    static bool isStringLikeKind(System::Typinfo::TTypeKind k) {
       using namespace System::Typinfo;
       return k == TTypeKind::tkUString || k == TTypeKind::tkLString
           || k == TTypeKind::tkWString || k == TTypeKind::tkString;
       }

    static void setViaTypInfo(System::Classes::TComponent * pComp,
                              System::UnicodeString const & uPropName,
                              System::UnicodeString const & u) {
       // klassischer, sehr robuster Weg über TypInfo
       ::SetPropValue(pComp, uPropName, u);
       }


    // Holt die Property aus einem frischen, lokalen Kontext
    System::Rtti::TRttiProperty * resolve() const {
       if(!pComp) return nullptr;
       System::Rtti::TRttiContext ctx;
       System::Rtti::TRttiType *const pType = ctx.GetType(pComp->ClassType());
       if(!pType) return nullptr;
       return pType->GetProperty(uPropName);
       }

    System::Classes::TComponent *pComp{};
    System::UnicodeString uPropName{};
 };

inline bool IsStringProperty(System::Rtti::TRttiProperty *const pProp) {
   if(pProp == nullptr) return false;
   System::Rtti::TRttiType *const pT = pProp->PropertyType;
   if(pT == nullptr) return false;
   using System::Typinfo::TTypeKind;
   switch(pT->TypeKind) {
	  case TTypeKind::tkUString:
	  case TTypeKind::tkWString:
	  case TTypeKind::tkLString:
	  case TTypeKind::tkString:
		 return true;
	  default:
		 return false;
	  }
   }

inline std::optional<StringPropertyRef> TryGetStringProperty(System::Classes::TComponent *const pComp, std::string const & strPropName) {
   if(pComp == nullptr) return std::nullopt;

   System::Rtti::TRttiContext aCtx;
   System::Rtti::TRttiType *const pType = aCtx.GetType(pComp->ClassType());
   if(pType == nullptr) return std::nullopt;
/*
   System::UnicodeString const theName = System::UnicodeString(System::AnsiString(strPropName.c_str()));
   System::Rtti::TRttiProperty *const pProp = pType->GetProperty(theName);
   if(pProp == nullptr) return std::nullopt;
   if(!pProp->IsReadable) return std::nullopt;
   if(!IsStringProperty(pProp)) return std::nullopt;

   return StringPropertyRef{pComp, pProp};
*/
    System::UnicodeString const uName = System::UnicodeString(System::AnsiString(strPropName.c_str()));
    System::Rtti::TRttiProperty *const pProp = pType->GetProperty(uName);
    if(pProp == nullptr) return std::nullopt;
    if(!pProp->IsReadable) return std::nullopt;
    if(!IsStringProperty(pProp)) return std::nullopt;

    // Wichtig: wir geben eine Ref mit (pComp, Property-Name) zurück – kein Property-Pointer!
    return StringPropertyRef{pComp, uName};
   }

inline StringPropertyRef
GetStringPropertyOrThrow(System::Classes::TComponent *const pComp, std::string const & strPropName) {
   if(auto const opt = TryGetStringProperty(pComp, strPropName)) {
	  return *opt;
      }
   throw std::runtime_error("GetStringPropertyOrThrow: property '" + strPropName + "' not available as string");
   }

inline StringPropertyRef
GetStringPropertyOrThrow(System::Classes::TComponent *const pComp, std::vector<std::string> const & vecPropName) {
   for(auto const& strPropName : vecPropName) {
      if(auto const opt = TryGetStringProperty(pComp, strPropName)) {
	     return *opt;
         }
      }
   throw std::runtime_error("GetStringPropertyOrThrow: property not available as string");
   }

} // end of namespace adecc::delphi

