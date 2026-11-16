#pragma once

#if defined(BUILD_WITH_VCL) && defined(BUILD_WITH_FMX)
   #error BUILD_WITH_VCL and BUILD_WITH_FMX are mutually exclusive.
#endif

#include "text_wrapper.h"
#include "convert_core.h"

#include <System.Classes.hpp>
#include <System.SysUtils.hpp>
#include <System.Types.hpp>

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string_view>

#if defined(BUILD_WITH_VCL)
   #include <Vcl.StdCtrls.hpp>
   #include <Vcl.ComCtrls.hpp>
#endif

#if defined(BUILD_WITH_FMX)
   #include <FMX.Memo.hpp>
   #include <FMX.ListBox.hpp>
   #include <FMX.Text.hpp>
#endif

namespace adecc::text {

namespace details {

inline void CheckLines(System::Classes::TStrings* const pLines, char const* const szWhere) {
   if(pLines == nullptr) {
      throw std::invalid_argument{ szWhere };
      }
   }

inline int ToInt(std::size_t const uValue, char const* const szWhere) {
   if(uValue > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
      throw std::out_of_range{ szWhere };
      }

   return static_cast<int>(uValue);
   }

template <adecc::StreamPolicy stream_ty>
class TStringsTextBackendBase {
public:
   using stream_policy_type = stream_ty;
   using char_type = typename stream_ty::char_type;
   using traits_type = typename stream_ty::traits_type;
   using string_type = typename stream_ty::string_type;
   using string_view_type = std::basic_string_view<char_type, traits_type>;

protected:
   explicit TStringsTextBackendBase(System::Classes::TStrings* const pLines) : pLines_{ pLines } {
      CheckLines(pLines_, "TStringsTextBackendBase: Lines/Items is null");
      }

public:
   [[nodiscard]] std::size_t lines() const {
      return static_cast<std::size_t>(lines_().Count);
      }

   [[nodiscard]] string_type get_line(std::size_t const uLine) const {
      return ConvertTo<string_type>(lines_().Strings[ToInt(uLine, "get_line: index too large")]);
      }

   void set_line(std::size_t const uLine, string_view_type const svText) {
      lines_().Strings[ToInt(uLine, "set_line: index too large")] = ConvertTo<System::UnicodeString>(svText);
      }

   void insert_line(std::size_t const uLine, string_view_type const svText) {
      lines_().Insert(ToInt(uLine, "insert_line: index too large"), ConvertTo<System::UnicodeString>(svText));
      }

   std::size_t append_line(string_view_type const svText) {
      return static_cast<std::size_t>(lines_().Add(ConvertTo<System::UnicodeString>(svText)));
      }

   void erase_line(std::size_t const uLine) {
      lines_().Delete(ToInt(uLine, "erase_line: index too large"));
      }

   void reset() {
      lines_().Clear();
      }

   void reset(bool const) {
      reset();
      }

   [[nodiscard]] bool freeze() {
      lines_().BeginUpdate();
      return true;
      }

   void unfreeze(bool const bFrozen) {
      if(bFrozen) {
         lines_().EndUpdate();
         }
      }

protected:
   [[nodiscard]] System::Classes::TStrings& lines_() const {
      CheckLines(pLines_, "TStringsTextBackendBase: Lines/Items is null");
      return *pLines_;
      }

private:
   System::Classes::TStrings* pLines_ = nullptr;
   };

} // namespace details

#if defined(BUILD_WITH_VCL)

template <adecc::StreamPolicy stream_ty = adecc::AnsiStreamPolicy>
class VclMemoTextBackend : public details::TStringsTextBackendBase<stream_ty> {
private:
   using Base = details::TStringsTextBackendBase<stream_ty>;

public:
   using typename Base::stream_policy_type;
   using typename Base::char_type;
   using typename Base::string_type;
   using typename Base::string_view_type;

   explicit VclMemoTextBackend(Vcl::Stdctrls::TCustomMemo* const pMemo)
      : Base{ pMemo != nullptr ? pMemo->Lines : nullptr }, pMemo_{ pMemo } {
      if(pMemo_ == nullptr) {
         throw std::invalid_argument{ "VclMemoTextBackend: Memo is null" };
         }
      }

   [[nodiscard]] std::size_t current_line() const {
      return static_cast<std::size_t>(pMemo_->CaretPos.y);
      }

   bool focus_line(std::size_t const uLine) {
      if(uLine >= this->lines()) {
         return false;
         }

      pMemo_->CaretPos = System::Types::TPoint{ 0, details::ToInt(uLine, "focus_line: index too large") };
      pMemo_->SetFocus();
      return true;
      }

private:
   Vcl::Stdctrls::TCustomMemo* pMemo_ = nullptr;
   };


template <adecc::StreamPolicy stream_ty = adecc::AnsiStreamPolicy>
class VclRichEditTextBackend : public details::TStringsTextBackendBase<stream_ty> {
private:
   using Base = details::TStringsTextBackendBase<stream_ty>;

public:
   using typename Base::stream_policy_type;
   using typename Base::char_type;
   using typename Base::string_type;
   using typename Base::string_view_type;

   explicit VclRichEditTextBackend(Vcl::Comctrls::TCustomRichEdit* const pRichEdit)
      : Base{ pRichEdit != nullptr ? pRichEdit->Lines : nullptr }, pRichEdit_{ pRichEdit } {
      if(pRichEdit_ == nullptr) {
         throw std::invalid_argument{ "VclRichEditTextBackend: RichEdit is null" };
         }
      }

   [[nodiscard]] std::size_t current_line() const {
      return static_cast<std::size_t>(pRichEdit_->CaretPos.y);
      }

   bool focus_line(std::size_t const uLine) {
      if(uLine >= this->lines()) {
         return false;
         }

      pRichEdit_->CaretPos = System::Types::TPoint{ 0, details::ToInt(uLine, "focus_line: index too large") };
      pRichEdit_->SetFocus();
      return true;
      }

private:
   Vcl::Comctrls::TCustomRichEdit* pRichEdit_ = nullptr;
   };


template <adecc::StreamPolicy stream_ty = adecc::AnsiStreamPolicy>
class VclListBoxTextBackend : public details::TStringsTextBackendBase<stream_ty> {
private:
   using Base = details::TStringsTextBackendBase<stream_ty>;

public:
   using typename Base::stream_policy_type;
   using typename Base::char_type;
   using typename Base::string_type;
   using typename Base::string_view_type;

   explicit VclListBoxTextBackend(Vcl::Stdctrls::TCustomListBox* const pListBox)
      : Base{ pListBox != nullptr ? pListBox->Items : nullptr }, pListBox_{ pListBox } {
      if(pListBox_ == nullptr) {
         throw std::invalid_argument{ "VclListBoxTextBackend: ListBox is null" };
         }
      }

   [[nodiscard]] std::size_t current_line() const {
      return static_cast<std::size_t>(pListBox_->ItemIndex);
      }

   bool focus_line(std::size_t const uLine) {
      if(uLine >= this->lines()) {
         return false;
         }

      pListBox_->ItemIndex = details::ToInt(uLine, "focus_line: index too large");
      pListBox_->SetFocus();
      return true;
      }

private:
   Vcl::Stdctrls::TCustomListBox* pListBox_ = nullptr;
   };


template <adecc::StreamPolicy stream_ty = adecc::AnsiStreamPolicy>
class VclComboBoxTextBackend : public details::TStringsTextBackendBase<stream_ty> {
private:
   using Base = details::TStringsTextBackendBase<stream_ty>;

public:
   using typename Base::stream_policy_type;
   using typename Base::char_type;
   using typename Base::string_type;
   using typename Base::string_view_type;

   explicit VclComboBoxTextBackend(Vcl::Stdctrls::TCustomComboBox* const pComboBox)
      : Base{ pComboBox != nullptr ? pComboBox->Items : nullptr }, pComboBox_{ pComboBox } {
      if(pComboBox_ == nullptr) {
         throw std::invalid_argument{ "VclComboBoxTextBackend: ComboBox is null" };
         }
      }

   [[nodiscard]] std::size_t current_line() const {
      return static_cast<std::size_t>(pComboBox_->ItemIndex);
      }

   bool focus_line(std::size_t const uLine) {
      if(uLine >= this->lines()) {
         return false;
         }

      pComboBox_->ItemIndex = details::ToInt(uLine, "focus_line: index too large");
      pComboBox_->SetFocus();
      return true;
      }

private:
   Vcl::Stdctrls::TCustomComboBox* pComboBox_ = nullptr;
   };

#endif // BUILD_WITH_VCL

#if defined(BUILD_WITH_FMX)

template <adecc::StreamPolicy stream_ty = adecc::AnsiStreamPolicy>
class FmxMemoTextBackend : public details::TStringsTextBackendBase<stream_ty> {
private:
   using Base = details::TStringsTextBackendBase<stream_ty>;

public:
   using typename Base::stream_policy_type;
   using typename Base::char_type;
   using typename Base::string_type;
   using typename Base::string_view_type;

   explicit FmxMemoTextBackend(Fmx::Memo::TMemo* const pMemo)
      : Base{ pMemo != nullptr ? pMemo->Lines : nullptr }, pMemo_{ pMemo } {
      if(pMemo_ == nullptr) {
         throw std::invalid_argument{ "FmxMemoTextBackend: Memo is null" };
         }
      }

   [[nodiscard]] std::size_t current_line() const {
      return static_cast<std::size_t>(pMemo_->CaretPosition.Line);
      }

   bool focus_line(std::size_t const uLine) {
      if(uLine >= this->lines()) {
         return false;
         }

	  pMemo_->CaretPosition = Fmx::Text::TCaretPosition{ 0, details::ToInt(uLine, "focus_line: index too large") };pMemo_->SetFocus();
      return true;
      }

private:
   Fmx::Memo::TMemo* pMemo_ = nullptr;
   };


template <adecc::StreamPolicy stream_ty = adecc::AnsiStreamPolicy>
class FmxListBoxTextBackend : public details::TStringsTextBackendBase<stream_ty> {
private:
   using Base = details::TStringsTextBackendBase<stream_ty>;

public:
   using typename Base::stream_policy_type;
   using typename Base::char_type;
   using typename Base::string_type;
   using typename Base::string_view_type;

   explicit FmxListBoxTextBackend(Fmx::Listbox::TCustomListBox* const pListBox)
      : Base{ pListBox != nullptr ? pListBox->Items : nullptr }, pListBox_{ pListBox } {
      if(pListBox_ == nullptr) {
         throw std::invalid_argument{ "FmxListBoxTextBackend: ListBox is null" };
         }
      }

   [[nodiscard]] std::size_t current_line() const {
      return static_cast<std::size_t>(pListBox_->ItemIndex);
      }

   bool focus_line(std::size_t const uLine) {
      if(uLine >= this->lines()) {
         return false;
         }

      pListBox_->ItemIndex = details::ToInt(uLine, "focus_line: index too large");
      pListBox_->SetFocus();
      return true;
      }

private:
   Fmx::Listbox::TCustomListBox* pListBox_ = nullptr;
   };


template <adecc::StreamPolicy stream_ty = adecc::AnsiStreamPolicy>
class FmxComboBoxTextBackend : public details::TStringsTextBackendBase<stream_ty> {
private:
   using Base = details::TStringsTextBackendBase<stream_ty>;

public:
   using typename Base::stream_policy_type;
   using typename Base::char_type;
   using typename Base::string_type;
   using typename Base::string_view_type;

   explicit FmxComboBoxTextBackend(Fmx::Listbox::TComboBox* const pComboBox)
      : Base{ pComboBox != nullptr ? pComboBox->Items : nullptr }, pComboBox_{ pComboBox } {
      if(pComboBox_ == nullptr) {
         throw std::invalid_argument{ "FmxComboBoxTextBackend: ComboBox is null" };
         }
      }

   [[nodiscard]] std::size_t current_line() const {
      return static_cast<std::size_t>(pComboBox_->ItemIndex);
      }

   bool focus_line(std::size_t const uLine) {
      if(uLine >= this->lines()) {
         return false;
         }

      pComboBox_->ItemIndex = details::ToInt(uLine, "focus_line: index too large");
      pComboBox_->SetFocus();
      return true;
      }

private:
   Fmx::Listbox::TComboBox* pComboBox_ = nullptr;
   };

#endif // BUILD_WITH_FMX

} // namespace adecc::text

