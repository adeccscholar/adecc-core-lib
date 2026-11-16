#pragma once

#include <concepts>

#include <System.Classes.hpp>
#include <System.SysUtils.hpp>

namespace adecc::delphi {

struct LinesOwnerShim {
   System::Classes::TStrings * Lines{};
   };


template <typename ty>
concept DelphiComponent = std::derived_from<ty, System::Classes::TComponent>;

template <typename ty>
concept HasLinesProp =
   requires(ty* p) {
      { p->Lines } -> std::convertible_to<System::Classes::TStrings*>;
   };

template <typename ty>
concept HasItemsProp =
   requires(ty* p) {
      { p->Items } -> std::convertible_to<System::Classes::TStrings*>;
   };

// Owner hat LinesOwnerShim ODER ist Delphi-Komponente, und besitzt Lines ODER Items
template <typename ty>
concept HasLinesOrItems =
   (std::same_as<std::remove_cvref_t<ty>, LinesOwnerShim> || DelphiComponent<ty>) &&
   (HasLinesProp<ty> || HasItemsProp<ty>);


template <typename>
inline constexpr bool always_false_v = false;


template <typename ty>
concept HasReadableText =  DelphiComponent<ty> && 
           requires(ty* p) { { p->Text } -> std::convertible_to<System::UnicodeString>; };
   
template <typename ty>
concept HasReadableCaption = DelphiComponent<ty> && 
           requires(ty* p) { { p->Caption } -> std::convertible_to<System::UnicodeString>; };

template <typename ty>
concept HasReadableSimpleText = DelphiComponent<ty> && 
           requires(ty* p) { { p->SimpleText } -> std::convertible_to<System::UnicodeString>; };


template <typename ty>
concept HasReadableUnicodeStringProp = HasReadableText<ty> || HasReadableCaption<ty> || HasReadableSimpleText<ty>;

// --- Writable UnicodeString properties ---

template <typename ty>
concept HasWritableText       =  DelphiComponent<ty> && 
           requires(ty* p, System::UnicodeString u) { p->Text = u; };

template <typename ty>
concept HasWritableCaption    = DelphiComponent<ty> &&  
           requires(ty* p, System::UnicodeString u) { p->Caption = u; };

template <typename ty>
concept HasWritableSimpleText = DelphiComponent<ty> && 
           requires(ty* p, System::UnicodeString u) { p->SimpleText = u; };

template <typename ty>
concept HasWritableUnicodeStringProp = HasWritableText<ty> || HasWritableCaption<ty> || HasWritableSimpleText<ty>;

// --- Zusammenfassend (les-/schreibbar) ---

template <typename ty>
concept HasUnicodeStringProp = HasReadableUnicodeStringProp<ty> && HasWritableUnicodeStringProp<ty>;


// ===========================================================================

template <typename ty>
concept HasReadableCheckedProp = DelphiComponent<ty> &&  
   requires(ty* p) {
      { p->Checked } -> std::convertible_to<bool>;
   };

template <typename ty>
concept HasWritableCheckedProp    = DelphiComponent<ty> &&  
           requires(ty* p, bool b) { p->Checked = b; };

template <typename ty>
concept HasReadableIsCheckedProp = DelphiComponent<ty> &&  
   requires(ty* p) {
      { p->IsChecked } -> std::convertible_to<bool>;
   };

template <typename ty>
concept HasWritableIsCheckedProp    = DelphiComponent<ty> &&  
           requires(ty* p, bool b) { p->IsChecked = b; };


   
template <typename ty>
concept HasReadableBoolProp = HasReadableCheckedProp<ty> || HasReadableIsCheckedProp<ty>;

   
template <typename ty>
concept HasWritableBoolProp = HasWritableCheckedProp<ty> || HasReadableIsCheckedProp<ty>;

   
template <typename ty>
concept HasBoolProp = HasReadableBoolProp<ty> && HasWritableBoolProp<ty>;
   
// ------------------
   
template <typename ty>
concept HasReadableEnabledProp = DelphiComponent<ty> &&  
   requires(ty* p) {
      { p->Enabled } -> std::convertible_to<bool>;
   };

template <typename ty>
concept HasWritableEnabledProp    = DelphiComponent<ty> &&  
           requires(ty* p, bool b) { p->Enabled = b; };

   
   
template <typename ty>
concept HasEnabledProp = HasReadableEnabledProp<ty> && HasWritableEnabledProp<ty>;
      
// =========================================================================

template <typename R, typename U>
concept EnumWithUnderlying =
   std::is_enum_v<std::remove_cvref_t<R>> &&
   std::same_as<std::underlying_type_t<std::remove_cvref_t<R>>, U>;


template <typename ty>
concept DelphiForm = std::derived_from<ty, System::Classes::TComponent> &&
                     HasWritableCaption<ty> &&
                     requires(ty* p) {
                               { p->Show() }  -> std::same_as<void>;
                               {  p->ShowModal() } -> std::convertible_to<int>;  } &&

                     ( requires(ty* p) { { p->Close() }      -> std::convertible_to<unsigned char>; } ||
                       requires(ty* p) { { p->Close() }      -> std::same_as<void>; } ||
                       requires(ty* p) { EnumWithUnderlying<decltype(p->Close()), unsigned char>; } ) &&

                     ( requires(ty* p) { { p->CloseQuery() } -> std::convertible_to<bool>; } ||
                       requires(ty* p) { { p->CanClose()   } -> std::convertible_to<bool>; } ||
                       requires(ty* p) { { p->CanClose()   } -> std::convertible_to<int>;  } ) &&

                     requires(ty* p, System::UnicodeString const& nm) {
                               { p->FindComponent(nm) } -> std::same_as<System::Classes::TComponent*>; };
                          

} // end of namespace adecc::delphi

