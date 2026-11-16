#pragma once

#include "stream_tools.h"
#include "delphi_concepts.h"

#include <System.hpp>               
#include <System.Rtti.hpp>
#include <System.TypInfo.hpp>
#include <System.Classes.hpp>



namespace adecc::delphi {

template <typename ty> requires HasReadableBoolProp<ty>
inline bool get_bool_value(ty* p) {
   if constexpr (HasReadableCheckedProp<ty>) return p->Checked;
   else if constexpr (HasReadableIsCheckedProp<ty>) return p->IsChecked;
   else static_assert(always_false_v<ty>, "Owner has neither Checked nor IsChecked");
   }

template <typename ty> requires HasWritableBoolProp<ty>
inline void set_bool_value(ty* p, bool b) {
   if constexpr (HasWritableCheckedProp<ty>) p->Checked = b;
   else if constexpr (HasWritableIsCheckedProp<ty>) p->IsChecked = b;
   else static_assert(always_false_v<ty>, "Owner has neither Checked nor IsChecked");
   }


template <typename ty> requires HasReadableEnabledProp<ty>
inline bool get_enabled(ty* p) {
   if constexpr (HasReadableEnabledProp<ty>) return p->Enabled;
   else static_assert(always_false_v<ty>, "Owner hasn't enabled");
   }

template <typename ty> requires HasWritableEnabledProp<ty>
inline void set_enabled(ty* p, bool b) {
   if constexpr (HasWritableEnabledProp<ty>) p->Enabled = b;
   else static_assert(always_false_v<ty>, "Owner hasn't enabled");
   }


   

// Get als UnicodeString (falls weder Text noch Caption vorhanden: ill-formed)
template <typename ty> requires HasReadableUnicodeStringProp<ty>
inline std::string get_str(ty* p) {
   if constexpr (HasReadableText<ty>) return AnsiString(p->Text).c_str();
   else                               return AnsiString(p->Caption).c_str(); // HasReadableCaption garantiert
   }

// Get als UnicodeString (falls weder Text noch Caption vorhanden: ill-formed)
template <typename ty> requires HasReadableUnicodeStringProp<ty>
inline System::UnicodeString get_ustr(ty* p) {
   if constexpr (HasReadableText<ty>) return p->Text;
   else                               return p->Caption; // HasReadableCaption garantiert
   }

// Set aus std::string
template <typename ty> requires HasWritableUnicodeStringProp<ty>
inline void set_ustr(ty* p, std::string const& s) {
   if constexpr (HasWritableText<ty>) p->Text = s.c_str();
   else if constexpr (HasWritableCaption<ty>)  p->Caption = s.c_str(); 
   else p->SimpleText = s.c_str();
   }



inline System::Classes::TStrings* tryGetStringList(System::Classes::TComponent *const pComp, std::string const& pName) {
   if(pComp == nullptr) return nullptr;
   else [[likely]] {
	  System::Rtti::TRttiContext ctx;
	  System::Rtti::TRttiType *const pType = ctx.GetType(pComp->ClassType());
	  if(pType == nullptr) return nullptr;
	  else [[likely]] {
		 System::Rtti::TRttiProperty *const pProp = pType->GetProperty(pName.c_str());
		 if(pProp == nullptr || !pProp->IsReadable) {
			return nullptr;
			}
		 else [[likely]] {
			System::Rtti::TValue v = pProp->GetValue(pComp);
			if(!v.IsObject()) return nullptr;
			else return dynamic_cast<System::Classes::TStrings*>(v.AsObject());
			}
		 }
	  }
   }


inline System::Classes::TComponent* findByName(System::Classes::TComponent *const pOwner, std::string const & strName) {
   if(pOwner == nullptr) return nullptr;
   return pOwner->FindComponent(strName.c_str());
   }


} // end of namespace adecc::delphi

