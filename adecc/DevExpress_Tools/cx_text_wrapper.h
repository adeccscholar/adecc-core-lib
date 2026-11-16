#pragma once

#if defined(BUILD_WITH_FMX)
   #error cx_text_wrapper_corrected.h is VCL/DevExpress only. BUILD_WITH_FMX is not supported here.
#endif

#if !defined(BUILD_WITH_VCL)
   #error cx_text_wrapper_corrected.h requires BUILD_WITH_VCL.
#endif

#include "text_wrapper.h"
#include "convert_core.h"

#include <System.Classes.hpp>
#include <System.SysUtils.hpp>
#include <System.Types.hpp>

#include <cxMemo.hpp>
#include <cxListBox.hpp>
#include <cxDropDownEdit.hpp>
#include <cxRichEdit.hpp>
#include <cxImageComboBox.hpp>

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace adecc::text {

namespace cx_details {

inline int ToInt(std::size_t const uValue, char const* const szWhere) {
   if(uValue > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
      throw std::out_of_range{ szWhere };
      }

   return static_cast<int>(uValue);
   }

inline void CheckLines(System::Classes::TStrings* const pLines, char const* const szWhere) {
   if(pLines == nullptr) {
      throw std::invalid_argument{ szWhere };
      }
   }

template <adecc::StreamPolicy stream_ty>
class TStringsTextBackendBase {
public:
   using stream_policy_type = stream_ty;
   using traits_type = typename stream_ty::traits_type;
   using char_type = typename stream_ty::char_type;
   using string_type = typename stream_ty::string_type;
   using string_view_type = std::basic_string_view<char_type, traits_type>;

protected:
   explicit TStringsTextBackendBase(System::Classes::TStrings* const pLines) : pLines_{ pLines } {
      CheckLines(pLines_, "TStringsTextBackendBase: TStrings is null");
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

   [[nodiscard]] std::size_t append_line(string_view_type const svText) {
      int const iLine = lines_().Add(ConvertTo<System::UnicodeString>(svText));
      return static_cast<std::size_t>(iLine);
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
      CheckLines(pLines_, "TStringsTextBackendBase: TStrings is null");
      return *pLines_;
      }

private:
   System::Classes::TStrings* pLines_ = nullptr;
   };

template <adecc::StreamPolicy stream_ty>
class TcxImageComboBoxTextBackendBase {
public:
   using stream_policy_type = stream_ty;
   using traits_type = typename stream_ty::traits_type;
   using char_type = typename stream_ty::char_type;
   using string_type = typename stream_ty::string_type;
   using string_view_type = std::basic_string_view<char_type, traits_type>;

protected:
   explicit TcxImageComboBoxTextBackendBase(Cximagecombobox::TcxImageComboBoxItems* const pItems) : pItems_{ pItems } {
      if(pItems_ == nullptr) {
         throw std::invalid_argument{ "TcxImageComboBoxTextBackendBase: items is null" };
         }
      }

public:
   [[nodiscard]] std::size_t lines() const {
      return static_cast<std::size_t>(items_().Count);
      }

   [[nodiscard]] string_type get_line(std::size_t const uLine) const {
      return ConvertTo<string_type>(items_().Items[ToInt(uLine, "get_line: index too large")]->Description);
      }

   void set_line(std::size_t const uLine, string_view_type const svText) {
      items_().Items[ToInt(uLine, "set_line: index too large")]->Description = ConvertTo<System::UnicodeString>(svText);
      }

   void insert_line(std::size_t const uLine, string_view_type const svText) {
      auto* pItem = items_().Insert(ToInt(uLine, "insert_line: index too large"));
      pItem->Description = ConvertTo<System::UnicodeString>(svText);
      }

   [[nodiscard]] std::size_t append_line(string_view_type const svText) {
      auto* pItem = items_().Add();
      pItem->Description = ConvertTo<System::UnicodeString>(svText);
      return static_cast<std::size_t>(items_().Count - 1);
      }

   void erase_line(std::size_t const uLine) {
      items_().Delete(ToInt(uLine, "erase_line: index too large"));
      }

   void reset() {
      items_().Clear();
      }

   void reset(bool const) {
      reset();
      }

   [[nodiscard]] bool freeze() {
      items_().BeginUpdate();
      return true;
      }

   void unfreeze(bool const bFrozen) {
      if(bFrozen) {
         items_().EndUpdate();
         }
      }

protected:
   [[nodiscard]] Cximagecombobox::TcxImageComboBoxItems& items_() const {
      if(pItems_ == nullptr) {
         throw std::invalid_argument{ "TcxImageComboBoxTextBackendBase: items is null" };
         }

      return *pItems_;
      }

private:
   Cximagecombobox::TcxImageComboBoxItems* pItems_ = nullptr;
   };

} // namespace cx_details


template <adecc::StreamPolicy stream_ty = adecc::AnsiStreamPolicy>
class CxMemoTextBackend : public cx_details::TStringsTextBackendBase<stream_ty> {
private:
   using Base = cx_details::TStringsTextBackendBase<stream_ty>;

public:
   using typename Base::stream_policy_type;
   using typename Base::char_type;
   using typename Base::string_type;
   using typename Base::string_view_type;

   explicit CxMemoTextBackend(Cxmemo::TcxCustomMemo* const pMemo)
      : Base{ pMemo != nullptr ? pMemo->Lines : nullptr }, pMemo_{ pMemo } {
      if(pMemo_ == nullptr) {
         throw std::invalid_argument{ "CxMemoTextBackend: memo is null" };
         }
      }

   [[nodiscard]] std::size_t current_line() const {
      return pMemo_->CaretPos.y < 0 ? std::size_t{} : static_cast<std::size_t>(pMemo_->CaretPos.y);
      }

   bool focus_line(std::size_t const uLine) {
      if(uLine >= this->lines()) {
         return false;
         }

      pMemo_->CaretPos = System::Types::TPoint{ 0, cx_details::ToInt(uLine, "focus_line: index too large") };
      pMemo_->SetFocus();
      return true;
      }

private:
   Cxmemo::TcxCustomMemo* pMemo_ = nullptr;
   };


template <adecc::StreamPolicy stream_ty = adecc::AnsiStreamPolicy>
class CxRichEditTextBackend : public cx_details::TStringsTextBackendBase<stream_ty> {
private:
   using Base = cx_details::TStringsTextBackendBase<stream_ty>;

public:
   using typename Base::stream_policy_type;
   using typename Base::char_type;
   using typename Base::string_type;
   using typename Base::string_view_type;

   explicit CxRichEditTextBackend(Cxrichedit::TcxCustomRichEdit* const pRichEdit)
      : Base{ pRichEdit != nullptr ? pRichEdit->Lines : nullptr }, pRichEdit_{ pRichEdit } {
      if(pRichEdit_ == nullptr) {
         throw std::invalid_argument{ "CxRichEditTextBackend: rich edit is null" };
         }
      }

   [[nodiscard]] std::size_t current_line() const {
      return pRichEdit_->CaretPos.y < 0 ? std::size_t{} : static_cast<std::size_t>(pRichEdit_->CaretPos.y);
      }

   bool focus_line(std::size_t const uLine) {
      if(uLine >= this->lines()) {
         return false;
         }

      pRichEdit_->CaretPos = System::Types::TPoint{ 0, cx_details::ToInt(uLine, "focus_line: index too large") };
      pRichEdit_->SetFocus();
      return true;
      }

private:
   Cxrichedit::TcxCustomRichEdit* pRichEdit_ = nullptr;
   };


template <adecc::StreamPolicy stream_ty = adecc::AnsiStreamPolicy>
class CxListBoxTextBackend : public cx_details::TStringsTextBackendBase<stream_ty> {
private:
   using Base = cx_details::TStringsTextBackendBase<stream_ty>;

public:
   using typename Base::stream_policy_type;
   using typename Base::char_type;
   using typename Base::string_type;
   using typename Base::string_view_type;

   explicit CxListBoxTextBackend(Cxlistbox::TcxListBox* const pListBox)
      : Base{ pListBox != nullptr ? pListBox->Items : nullptr }, pListBox_{ pListBox } {
      if(pListBox_ == nullptr) {
         throw std::invalid_argument{ "CxListBoxTextBackend: list box is null" };
         }
      }

   [[nodiscard]] std::size_t current_line() const {
      return pListBox_->ItemIndex < 0 ? std::size_t{} : static_cast<std::size_t>(pListBox_->ItemIndex);
      }

   bool focus_line(std::size_t const uLine) {
      if(uLine >= this->lines()) {
         return false;
         }

      pListBox_->ItemIndex = cx_details::ToInt(uLine, "focus_line: index too large");
      pListBox_->SetFocus();
      return true;
      }

private:
   Cxlistbox::TcxListBox* pListBox_ = nullptr;
   };


template <adecc::StreamPolicy stream_ty = adecc::AnsiStreamPolicy>
class CxComboBoxTextBackend : public cx_details::TStringsTextBackendBase<stream_ty> {
private:
   using Base = cx_details::TStringsTextBackendBase<stream_ty>;

public:
   using typename Base::stream_policy_type;
   using typename Base::char_type;
   using typename Base::string_type;
   using typename Base::string_view_type;

   explicit CxComboBoxTextBackend(Cxdropdownedit::TcxCustomComboBox* const pComboBox)
      : Base{ pComboBox != nullptr && pComboBox->Properties != nullptr ? pComboBox->Properties->Items : nullptr }, pComboBox_{ pComboBox } {
      if(pComboBox_ == nullptr) {
         throw std::invalid_argument{ "CxComboBoxTextBackend: combo box is null" };
         }
      if(pComboBox_->Properties == nullptr) {
         throw std::invalid_argument{ "CxComboBoxTextBackend: combo box properties is null" };
         }
      }

   [[nodiscard]] std::size_t current_line() const {
      return pComboBox_->ItemIndex < 0 ? std::size_t{} : static_cast<std::size_t>(pComboBox_->ItemIndex);
      }

   bool focus_line(std::size_t const uLine) {
      if(uLine >= this->lines()) {
         return false;
         }

      pComboBox_->ItemIndex = cx_details::ToInt(uLine, "focus_line: index too large");
      pComboBox_->SetFocus();
      return true;
      }

private:
   Cxdropdownedit::TcxCustomComboBox* pComboBox_ = nullptr;
   };


template <adecc::StreamPolicy stream_ty = adecc::AnsiStreamPolicy>
class CxImageComboBoxTextBackend : public cx_details::TcxImageComboBoxTextBackendBase<stream_ty> {
private:
   using Base = cx_details::TcxImageComboBoxTextBackendBase<stream_ty>;

public:
   using typename Base::stream_policy_type;
   using typename Base::char_type;
   using typename Base::string_type;
   using typename Base::string_view_type;

   explicit CxImageComboBoxTextBackend(Cximagecombobox::TcxCustomImageComboBox* const pComboBox)
      : Base{ pComboBox != nullptr && pComboBox->Properties != nullptr ? pComboBox->Properties->Items : nullptr }, pComboBox_{ pComboBox } {
      if(pComboBox_ == nullptr) {
         throw std::invalid_argument{ "CxImageComboBoxTextBackend: image combo box is null" };
         }
      if(pComboBox_->Properties == nullptr) {
         throw std::invalid_argument{ "CxImageComboBoxTextBackend: image combo box properties is null" };
         }
      }

   [[nodiscard]] std::size_t current_line() const {
      return pComboBox_->ItemIndex < 0 ? std::size_t{} : static_cast<std::size_t>(pComboBox_->ItemIndex);
      }

   bool focus_line(std::size_t const uLine) {
      if(uLine >= this->lines()) {
         return false;
         }

      pComboBox_->ItemIndex = cx_details::ToInt(uLine, "focus_line: index too large");
      pComboBox_->SetFocus();
      return true;
      }

private:
   Cximagecombobox::TcxCustomImageComboBox* pComboBox_ = nullptr;
   };

static_assert(adecc::text::text_type<adecc::text::CxMemoTextBackend<>>);
static_assert(adecc::text::text_type<adecc::text::CxRichEditTextBackend<>>);
static_assert(adecc::text::text_type<adecc::text::CxListBoxTextBackend<>>);
static_assert(adecc::text::text_type<adecc::text::CxComboBoxTextBackend<>>);
static_assert(adecc::text::text_type<adecc::text::CxImageComboBoxTextBackend<>>);

} // namespace adecc::text

