#pragma once

#if defined(BUILD_WITH_VCL)

#include "grid_wrapper.h"
#include "stream_tools.h"
#include "scope.h"

#include "convert_core.h"
#include "delphi_convert.h"

#include <cxGrid.hpp>
#include <cxGridBandedTableView.hpp>
#include <cxGridCustomTableView.hpp>
#include <cxGridTableView.hpp>

#include <cxEdit.hpp>
#include <cxTextEdit.hpp>
#include <cxMemo.hpp>
#include <cxSpinEdit.hpp>
#include <cxCheckBox.hpp>
#include <cxCalendar.hpp>
#include <cxTimeEdit.hpp>

#include <cxGridCustomView.hpp>
#include "cxCustomData.hpp"
#include <cxData.hpp>

#include <cxEditRepositoryItems.hpp>
#include <cxImageComboBox.hpp>  // ??

#include <stdexcept>
#include <algorithm>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <unordered_map>

namespace adecc::delphi::vcl::devexpress {


enum class EGridColType : uint32_t {
      text,
      memo,
      integer,
      floating,
      currency,
      percent, 
      count,
      checkbox,
      combobox,
      boolean,
      date,
      datetime,
      time
      };

enum class EGridColAggregate {
      none,
      count,
      distinct_count,
      sum,
      average,
      min,
      max
      };

template <adecc::StreamPolicy ty = adecc::AnsiStreamPolicy>
using tplBandCaption = std::tuple<
      typename ty::string_type, // 0: Caption
      bool                      // 1: Visible
      >;

template <adecc::StreamPolicy ty = adecc::AnsiStreamPolicy>
using vecBandCaptions = std::vector<tplBandCaption<ty>>;
      
template <adecc::StreamPolicy ty = adecc::AnsiStreamPolicy>
using tplBandedCaption = std::tuple<
      typename ty::string_type, //  0: Caption
      int,                      //  1: Width
      EAlignmentType,           //  2: Alignment
      int,                      //  3: ColIndex
      int,                      //  4: BandIndex
      bool,                     //  5: Visible
      bool,                     //  6: Editable
      bool,                     //  7: Sortable
      bool,                     //  8: Filterable
      bool,                     //  9: Groupable
      bool,                     // 10: Searchable
      EGridColType,             // 11: ColumnType
      int,                      // 12: Scale
      EGridColAggregate,        // 13: Aggregate
      std::string               // 14: später für Repos
      >;

template <adecc::StreamPolicy ty = adecc::AnsiStreamPolicy>
using vecBandedCaptions = std::vector<tplBandedCaption<ty>>;

static_assert(CaptionVector<vecBandedCaptions<adecc::AnsiStreamPolicy>>, "Caption: Conditions not met.");

// -----------------------------------------------------------------------------
// ImageComboBox Repository Typen
// -----------------------------------------------------------------------------

template <adecc::Integral ty = int, adecc::StreamPolicy SP = adecc::AnsiStreamPolicy>
using tplImageComboValue = std::pair<ty, typename SP::string_type>;

template <adecc::Integral ty = int, adecc::StreamPolicy SP = adecc::AnsiStreamPolicy>
using vecImageComboValues = std::vector<tplImageComboValue<ty, SP>>;

template <adecc::Integral ty = int, adecc::StreamPolicy SP = adecc::AnsiStreamPolicy>
using tplImageComboRepoItem = std::tuple<
          std::string,                // 0: Name des Repository Items
          vecImageComboValues<ty, SP> // 1: Werte
          >;

template <adecc::Integral ty = int, adecc::StreamPolicy SP = adecc::AnsiStreamPolicy>
using vecImageComboRepoItems = std::vector<tplImageComboRepoItem<ty, SP>>;

template <adecc::Integral ty = int, adecc::StreamPolicy SP = adecc::AnsiStreamPolicy>
using tplImageComboRepository = std::tuple<
          std::string,                   // 0: Repository Name
          vecImageComboRepoItems<ty, SP> // 1: Repository Eintraege
          >;


   
class BandedTableViewBackend {
public:
   using size_type = std::size_t;

   template <adecc::StreamPolicy SP = adecc::AnsiStreamPolicy>
   explicit BandedTableViewBackend(::TcxGridBandedTableView* pView,
                                   vecBandCaptions<SP> const& bandcaps = {})
                    : pView_{pView} {
      if(!pView_) {
         throw std::invalid_argument{"delphi::vcl::BandedTableViewBackend: null"};
         }

      pEditRepository_ = nullptr;

      SetBands_<SP>(bandcaps);
      pView_->BeginUpdate();
      auto scope = adecc::makeScopeExit([&]() { pView_->EndUpdate(); });

      //pView_->ClearItems();
      
      pView_->OptionsSelection->CellSelect = true;
      pView_->OptionsData->Editing = true;
      //pView_->OptionsCustomize->BandsQuickCustomization = true;
      pView_->OptionsCustomize->ColumnsQuickCustomization = true;
      pView_->OptionsCustomize->ColumnsQuickCustomizationMaxDropDownCount = 15;
      pView_->OptionsCustomize->ColumnMoving = true;
      pView_->OptionsCustomize->BandMoving = false;
      pView_->OptionsCustomize->ColumnFiltering = true;
      pView_->OptionsCustomize->ColumnSorting = true;
      pView_->OptionsCustomize->ColumnGrouping = true;

      pView_->Filtering->ColumnPopupMode = fpmExcel;
      //pView_->Filtering->ColumnExcelPopup->DateTimeValuesPageType = dvptTree;

      pView_->OptionsView->BandHeaders = false;   /// !!!
      pView_->OptionsView->ColumnAutoWidth = false;   
      pView_->OptionsView->CellAutoHeight = true;

      pView_->OptionsSelection->CellSelect    = false;
      pView_->OptionsSelection->HideSelection = false;
      }

   template <adecc::StreamPolicy SP = adecc::AnsiStreamPolicy, adecc::Integral ty = int>
   explicit BandedTableViewBackend(::TcxGridBandedTableView* pView,
								   vecBandCaptions<SP> const& bandcaps,
								   tplImageComboRepository<ty, SP> const& aRepository)
					 : BandedTableViewBackend{pView, bandcaps} {
	  CreateRepository_<ty, SP>(aRepository);
	  }

	/*
   // noch kümmern, eventuell tag type
   template <adecc::StreamPolicy SP = adecc::AnsiStreamPolicy>
   explicit BandedTableViewBackend(::TcxGridBandedTableView* pView,
								   adecc::vecCaptions<SP> const& caps,
								   bool bClear = true)
	  : BandedTableViewBackend{pView, vecBandCaptions<SP>{} } {
	  set_caption(caps, bClear);
	  }

   template <adecc::StreamPolicy SP = adecc::AnsiStreamPolicy>
   explicit BandedTableViewBackend(::TcxGridBandedTableView* pView,
								   vecBandCaptions<SP> const& bandcaps,
								   vecBandedCaptions<SP> const& caps,
								   bool bClear = true)
	  : BandedTableViewBackend{pView, bandcaps } {
	  set_caption(caps, bClear);
	  }
   */

   BandedTableViewBackend(BandedTableViewBackend const&) = delete;

   BandedTableViewBackend(BandedTableViewBackend&& rhs) noexcept
			  : pView_{rhs.pView_}, vecBands_{std::move(rhs.vecBands_)}
				, pEditRepository_{rhs.pEditRepository_}
				, mpImageComboRepositoryItems_{std::move(rhs.mpImageComboRepositoryItems_)}
				, iColumnWidthScalePx_{rhs.iColumnWidthScalePx_}
				, iColumnWidthPaddingPx_{rhs.iColumnWidthPaddingPx_} {
	  rhs.pView_ = nullptr;
	  rhs.pEditRepository_ = nullptr;
	  rhs.mpImageComboRepositoryItems_.clear();
	  }


   ~BandedTableViewBackend() {
	  DestroyRepository_();
	  }


   BandedTableViewBackend& operator=(BandedTableViewBackend const&) = delete;

   BandedTableViewBackend& operator=(BandedTableViewBackend&& rhs) noexcept {
	  if(this != &rhs) {
		 DestroyRepository_();

		 pView_ = rhs.pView_;
		 vecBands_ = std::move(rhs.vecBands_);
		 pEditRepository_ = rhs.pEditRepository_;
		 mpImageComboRepositoryItems_ = std::move(rhs.mpImageComboRepositoryItems_);
		 iColumnWidthScalePx_ = rhs.iColumnWidthScalePx_;
		 iColumnWidthPaddingPx_ = rhs.iColumnWidthPaddingPx_;

		 rhs.pView_ = nullptr;
		 rhs.pEditRepository_ = nullptr;
		 rhs.mpImageComboRepositoryItems_.clear();
		 }
	  return *this;
	  }


   void set_column_width_scale_px(int const iColumnWidthScalePx) {
      iColumnWidthScalePx_ = (std::max)(1, iColumnWidthScalePx);
      }

   [[nodiscard]] int column_width_scale_px() const noexcept {
      return iColumnWidthScalePx_;
      }

   void set_column_width_padding_px(int const iColumnWidthPaddingPx) {
      iColumnWidthPaddingPx_ = (std::max)(0, iColumnWidthPaddingPx);
      }

   [[nodiscard]] int column_width_padding_px() const noexcept {
      return iColumnWidthPaddingPx_;
      }

   // ------------------------------------------------------------------
   // Caption setzen: einfache Struktur
   // tuple<Hdr, Width, Align>
   // ------------------------------------------------------------------
   template <adecc::StreamPolicy SP = adecc::AnsiStreamPolicy>
   void set_caption(adecc::vecCaptions<SP> const& caps, bool bClear = true) {
	  try {
		 pView_->BeginUpdate();
		 auto scope = adecc::makeScopeExit([&]() { pView_->EndUpdate(); });
		 if(bClear) {
			Clear_();
			}

		 EnsureBands_(1);
		 ApplyBands_();

		 for(auto const& t : caps) {
			TcxGridBandedColumn* col = pView_->CreateColumn();
			if(!col) {
			   throw std::runtime_error{"set_caption(simple): CreateColumn failed"};
			   }

			col->Caption = adecc::ConvertTo<System::UnicodeString>(std::get<0>(t));
			col->Width   = ScaleColumnWidth_(static_cast<int>(std::get<1>(t)));
			col->Visible = true;

			col->Options->Editing  = false;
			col->Options->Sorting  = true;
			col->Options->Filtering = true;
			col->Options->Grouping = true;

			col->Position->BandIndex = 0;

			ApplyColumn_(pView_, col, MapAlignment_(std::get<2>(t)),
						 EGridColType::text, 0, EGridColAggregate::none);
			}
		 }
	  catch(Exception const& ex) {
		 throw std::runtime_error(AnsiString(ex.Message).c_str());
		 }
	  }

   // ------------------------------------------------------------------
   // tuple<Hdr, Width, Align, ColIndex, BandIndex, Visible, Editable,
   //       Sortable, Filterable, Groupable, Searchable,
   //       ColumnType, Scale, Aggregate, TagString>
   // ------------------------------------------------------------------
   template <adecc::StreamPolicy SP = adecc::AnsiStreamPolicy>
   void set_caption(vecBandedCaptions<SP> const& caps, bool bClear = true) {
	  try {
		 pView_->BeginUpdate();
		 auto scope = adecc::makeScopeExit([&]() { pView_->EndUpdate(); });

		 if(bClear) {
			Clear_();
			}

		 int iMaxBand = 0;
		 for(auto const& t : caps) {
			iMaxBand = (std::max)(iMaxBand, static_cast<int>(std::get<4>(t)));
			}

		 EnsureBands_(iMaxBand + 1);
		 ApplyBands_();

		 bool bHasGroupable  = false;
		 bool bHasSearchable = false;
		 bool bHasAggregate  = false;

		 for(auto const& t : caps) {
			auto* col = pView_->CreateColumn();
			if(!col) {
			   throw std::runtime_error{"set_caption(complex): CreateColumn failed"};
			   }

			TAlignment const eAlignment = MapAlignment_(std::get<2>(t));
			std::string const strTag = std::get<14>(t);

			col->Caption = adecc::ConvertTo<System::UnicodeString>(std::get<0>(t));
			col->Width   = ScaleColumnWidth_(static_cast<int>(std::get<1>(t)));

			col->Position->ColIndex  = static_cast<int>(std::get<3>(t));
			col->Position->BandIndex = static_cast<int>(std::get<4>(t));
			col->Position->RowIndex  = 0;

			col->Visible = std::get<5>(t);

			col->Options->Editing   = std::get<6>(t);
			col->Options->Sorting   = std::get<7>(t);
			col->Options->Filtering = std::get<8>(t);
			col->Options->Grouping  = std::get<9>(t);
			col->Options->IncSearch = std::get<10>(t);

			ApplyColumn_(col,
						 eAlignment,
						 std::get<11>(t),
						 std::get<12>(t),
						 std::get<13>(t),
						 strTag);

			bHasGroupable  = bHasGroupable  || std::get<9>(t);
			bHasSearchable = bHasSearchable || std::get<10>(t);
			bHasAggregate  = bHasAggregate  || (std::get<13>(t) != EGridColAggregate::none);
			}

		 pView_->OptionsView->GroupByBox        = bHasGroupable;
		 pView_->OptionsBehavior->IncSearch     = bHasSearchable;
		 pView_->OptionsView->Footer            = bHasAggregate;
		 pView_->OptionsView->GroupFooters      = bHasAggregate ? gfVisibleWhenExpanded : gfInvisible;
		 pView_->FindPanel->DisplayMode         = fpdmManual;
		 }
	  catch(Exception const& ex) {
		 throw std::runtime_error(AnsiString(ex.Message).c_str());
		 }
	  }

   // ------------------------------------------------------------------
   // Caption setzen: generische Tupel-Variante analog zum Original
   // ------------------------------------------------------------------
   template <class... Args> requires (sizeof...(Args) >= 3)
   void set_caption(std::vector<std::tuple<Args...>> const& caps, bool bClear = true) {
	  using tuple_ty = std::tuple<Args...>;

	  using caption_ty = std::remove_cvref_t<std::tuple_element_t<0, tuple_ty>>;
	  using width_ty   = std::remove_cvref_t<std::tuple_element_t<1, tuple_ty>>;
	  using align_ty   = std::remove_cvref_t<std::tuple_element_t<2, tuple_ty>>;

	  static_assert(requires(caption_ty const& aCaption) {
		 { adecc::ConvertTo<System::UnicodeString>(aCaption) } -> std::same_as<System::UnicodeString>;
		 }, "set_caption: tuple element 0 must be convertible to System::UnicodeString");

	  static_assert(std::is_integral_v<width_ty>,
			"set_caption: tuple element 1 must be an integral width");

	  static_assert(std::is_same_v<align_ty, EAlignmentType> || std::is_integral_v<align_ty>,
			"set_caption: tuple element 2 must be EAlignmentType or integral");

	  auto mapAlign = [](auto const& a) -> TAlignment {
		 using A = std::remove_cvref_t<decltype(a)>;

		 if constexpr (std::is_same_v<A, EAlignmentType>) {
			switch(a) {
			   case EAlignmentType::right:   return taRightJustify;
			   case EAlignmentType::center:  return taCenter;
			   case EAlignmentType::left:    [[fallthrough]];
			   case EAlignmentType::unknown: [[fallthrough]];
			   default:                      return taLeftJustify;
			   }
			}
		 else if constexpr (std::is_integral_v<A>) {
			switch(static_cast<int>(a)) {
			   case 2:  return taRightJustify;
			   case 3:  return taCenter;
			   case 1:  [[fallthrough]];
			   default: return taLeftJustify;
			   }
			}
		 else {
			return taLeftJustify;
			}
		 };

	  auto readBool = [](auto const& a, bool const bDefault) -> bool {
		 using A = std::remove_cvref_t<decltype(a)>;
		 if constexpr (std::is_same_v<A, bool>) {
			return a;
			}
		 else if constexpr (std::is_integral_v<A>) {
			return static_cast<int>(a) != 0;
			}
		 else {
			return bDefault;
			}
		 };

	  pView_->BeginUpdate();
	  if(bClear) {
		 Clear_();
		 }
	  pView_->EndUpdate();

	  pView_->BeginUpdate();
	  auto scope = adecc::makeScopeExit([&]() { pView_->EndUpdate(); });

	  int iMaxBand = 0;

	  if constexpr (sizeof...(Args) >= 5) {
		 using band_ty = std::remove_cvref_t<std::tuple_element_t<4, tuple_ty>>;
		 if constexpr (std::is_integral_v<band_ty>) {
			for(auto const& t : caps) {
			   iMaxBand = (std::max)(iMaxBand, static_cast<int>(std::get<4>(t)));
			   }
			}
		 }

	  {
		 int const iBandsFromCaps = iMaxBand + 1;
		 int const iBandsFromDefs = vecBands_.empty() ? 1 : static_cast<int>(vecBands_.size());
		 EnsureBands_((std::max)(iBandsFromCaps, iBandsFromDefs));
	  }

	  if(vecBands_.empty()) {
		 if(pView_->Bands->Count == 0) {
			EnsureBands_(1);
			}
		 for(int i = 0; i < pView_->Bands->Count; ++i) {
			auto* pBand = pView_->Bands->Items[i];
			if(!pBand) {
			   continue;
			   }
			pBand->Caption = System::UnicodeString{};
			pBand->Visible = (i == 0);
			pBand->Options->HoldOwnColumnsOnly = true;
			}
		 pView_->OptionsView->BandHeaders = false;
		 }
	  else {
		 ApplyBands_();
		 }

	  bool bHasGroupable  = false;
	  bool bHasSearchable = false;
	  bool bHasAggregate  = false;

	  int iAutoColIndex = 0;

	  for(auto const& t : caps) {
		 TcxGridBandedColumn* col = pView_->CreateColumn();
		 if(!col) {
			throw std::runtime_error{"set_caption(tuple): CreateColumn failed"};
			}

		 col->Caption = adecc::ConvertTo<System::UnicodeString>(std::get<0>(t));
		 col->Width   = ScaleColumnWidth_(static_cast<int>(std::get<1>(t)));

		 TAlignment const eAlignment = mapAlign(std::get<2>(t));
		 col->HeaderAlignmentHorz = eAlignment;

		 if constexpr (sizeof...(Args) >= 4) {
			using colindex_ty = std::remove_cvref_t<std::tuple_element_t<3, tuple_ty>>;
			if constexpr (std::is_integral_v<colindex_ty>) {
			   col->Position->ColIndex = static_cast<int>(std::get<3>(t));
			   }
			else {
			   col->Position->ColIndex = iAutoColIndex;
			   }
			}
		 else {
			col->Position->ColIndex = iAutoColIndex;
			}

		 if constexpr (sizeof...(Args) >= 5) {
			using band_ty = std::remove_cvref_t<std::tuple_element_t<4, tuple_ty>>;
			if constexpr (std::is_integral_v<band_ty>) {
			   col->Position->BandIndex = static_cast<int>(std::get<4>(t));
			   }
			else {
			   col->Position->BandIndex = 0;
			   }
			}
		 else {
			col->Position->BandIndex = 0;
			}

		 col->Position->RowIndex = 0;
		 ++iAutoColIndex;

		 bool bVisible      = true;
		 bool bEditable     = false;
		 bool bSortable     = true;
		 bool bFilterable   = true;
		 bool bGroupable    = true;
		 bool bSearchable   = true;
		 EGridColType eType = EGridColType::text;
		 EGridColAggregate eAggregate = EGridColAggregate::none;
		 int iScale = 0;
		 std::string strTag{};

		 if constexpr (sizeof...(Args) >= 6) {
			bVisible = readBool(std::get<5>(t), true);
			}

		 if constexpr (sizeof...(Args) >= 7) {
			bEditable = readBool(std::get<6>(t), false);
			}

		 if constexpr (sizeof...(Args) >= 8) {
			bSortable = readBool(std::get<7>(t), true);
			}

		 if constexpr (sizeof...(Args) >= 9) {
			bFilterable = readBool(std::get<8>(t), true);
			}

		 if constexpr (sizeof...(Args) >= 10) {
			bGroupable = readBool(std::get<9>(t), true);
			}

		 if constexpr (sizeof...(Args) >= 11) {
			bSearchable = readBool(std::get<10>(t), true);
			}

		 if constexpr (sizeof...(Args) >= 12) {
			using col_type_ty = std::remove_cvref_t<std::tuple_element_t<11, tuple_ty>>;
			if constexpr (std::is_same_v<col_type_ty, EGridColType>) {
			   eType = std::get<11>(t);
			   }
			}

		 if constexpr (sizeof...(Args) >= 13) {
			using scale_ty = std::remove_cvref_t<std::tuple_element_t<12, tuple_ty>>;
			if constexpr (std::is_integral_v<scale_ty>) {
			   iScale = static_cast<int>(std::get<12>(t));
			   }
			}

		 if constexpr (sizeof...(Args) >= 14) {
			using aggr_ty = std::remove_cvref_t<std::tuple_element_t<13, tuple_ty>>;
			if constexpr (std::is_same_v<aggr_ty, EGridColAggregate>) {
			   eAggregate = std::get<13>(t);
			   }
			}

		 if constexpr (sizeof...(Args) >= 15) {
			using tag_ty = std::remove_cvref_t<std::tuple_element_t<14, tuple_ty>>;
			if constexpr (std::is_same_v<tag_ty, std::string>) {
			   strTag = std::get<14>(t);
			   }
			else if constexpr (std::is_same_v<tag_ty, std::string_view>) {
			   strTag.assign(std::get<14>(t).begin(), std::get<14>(t).end());
			   }
			else if constexpr (std::is_same_v<tag_ty, char const*>) {
			   strTag = std::get<14>(t) ? std::get<14>(t) : "";
			   }
			}

		 col->Visible = bVisible;
		 col->Options->Editing   = bEditable;
		 col->Options->Sorting   = bSortable;
		 col->Options->Filtering = bFilterable;
		 col->Options->Grouping  = bGroupable;
		 col->Options->IncSearch = bSearchable;

		 ApplyColumn_(col, eAlignment, eType, iScale, eAggregate, strTag);

		 bHasGroupable  = bHasGroupable  || bGroupable;
		 bHasSearchable = bHasSearchable || bSearchable;
		 bHasAggregate  = bHasAggregate  || (eAggregate != EGridColAggregate::none);
		 }

	  pView_->OptionsView->GroupByBox        = bHasGroupable;
	  pView_->OptionsBehavior->IncSearch     = bHasSearchable;
	  pView_->OptionsView->Footer            = bHasAggregate;
	  pView_->OptionsView->GroupFooters      = bHasAggregate ? gfVisibleWhenExpanded : gfInvisible;
	  pView_->FindPanel->DisplayMode         = fpdmManual;
	  }


  template <adecc::StreamPolicy SP = adecc::AnsiStreamPolicy>
  void prepare_layout(vecBandCaptions<SP> const& theBands,
					  vecBandedCaptions<SP> const& theCaps,
					  bool const bClear = true) {
	  SetBands_<SP>(theBands);
	  set_caption<SP>(theCaps, bClear);
	  }


   template <adecc::StreamPolicy SP = adecc::AnsiStreamPolicy, adecc::Integral ty = int>
   void set_repository(tplImageComboRepository<ty, SP> const& aRepository) {
	  CreateRepository_<ty, SP>(aRepository);
	  }



   void reset(bool const bResetRepository = true) {
	  pView_->BeginUpdate();
	  auto scope = adecc::makeScopeExit([&]() { pView_->EndUpdate(); });

	  pView_->DataController->RecordCount = 0;

	  if(bResetRepository) {
		 pView_->ClearItems();
		 pView_->Bands->Clear();
		 vecBands_.clear();
         DestroyRepository_();

         pView_->OptionsView->BandHeaders    = false;
         pView_->OptionsView->GroupByBox     = false;
         pView_->OptionsView->Footer         = false;
         pView_->OptionsView->GroupFooters   = gfInvisible;
         pView_->OptionsBehavior->IncSearch  = false;
         }
      }

   [[nodiscard]] bool freeze() {
	  if(!pView_) {
		 return false;
		 }

	  pView_->BeginUpdate();
	  return true;
	  }

   void unfreeze(bool const bFrozen) {
	  if(bFrozen && pView_) {
		 pView_->EndUpdate();
		 }
	  }

   // ------------------------------------------------------------------
   // Dimensionen / aktuelle Position
   // ------------------------------------------------------------------
   size_type rows() const noexcept {
      return static_cast<size_type>(pView_->DataController->RecordCount);
      }

   size_type columns() const noexcept {
      return static_cast<size_type>(pView_->ColumnCount);
      }

   [[nodiscard]] size_type current_row() const noexcept {
      int const idx = pView_->DataController->FocusedRecordIndex;
      return idx >= 0 ? static_cast<size_type>(idx) : static_cast<size_type>(0);
      }

   [[nodiscard]] size_type current_column() const noexcept {
      TcxCustomGridTableItem* pItem = pView_->Controller->FocusedItem;
      return pItem ? static_cast<size_type>(pItem->Index) : static_cast<size_type>(0);
      }

bool focus_row(size_type const iRow) {
   if(iRow >= rows()) {
      return false;
      }

   int const iRecordIndex = static_cast<int>(iRow);

   if(pView_->Site) {
      pView_->Site->SetFocus();
      }

   // Datenindex -> sichtbarer Zeilenindex
   int const iRowIndex =
      pView_->DataController->GetRowIndexByRecordIndex(iRecordIndex, true);

   if(iRowIndex < 0) {
      return false;
      }

   pView_->BeginUpdate();
   auto scope = adecc::makeScopeExit([&]() { pView_->EndUpdate(); });

   pView_->Controller->FocusedRecordIndex = iRowIndex;

   if(iRowIndex >= pView_->ViewData->RecordCount) {
      return false;
      }

   auto* pRecord = pView_->ViewData->Records[iRowIndex];
   if(!pRecord || !pRecord->IsData) {
      return false;
      }

   pRecord->Focused = true;
   pView_->Controller->MakeFocusedRecordVisible();

   return true;
}

   // Neue Zeile anhaengen, Index zurueckgeben
   size_type insert_row() {
      int const iBefore = pView_->DataController->RecordCount;
      pView_->DataController->AppendRecord();
      return static_cast<size_type>(iBefore);
      }

   // ------------------------------------------------------------------
   // GET: get<std::optional<T>>(row,col) -> std::optional<T>
   // T muss in defined_values_types liegen
   // ------------------------------------------------------------------
   template <class opt_ty>
   requires (
      adecc::is_optional_v<opt_ty> &&
      adecc::is_in_type_list_v<typename opt_ty::value_type, adecc::defined_values_types>
      )
   opt_ty get(std::size_t iRow, std::size_t iCol) const {
      using T = typename opt_ty::value_type;
              
      std::size_t const nRows = static_cast<std::size_t>(pView_->DataController->RecordCount);
      std::size_t const nCols = static_cast<std::size_t>(pView_->ColumnCount);

      if(iRow >= nRows) throw std::out_of_range{"get: row"};
      if(iCol >= nCols) throw std::out_of_range{"get: col"};

      Variant const v = pView_->DataController->Values[static_cast<int>(iRow)][static_cast<int>(iCol)];
      if(VarIsNull(v) || VarIsEmpty(v)) {
         return opt_ty{};
         }

      return ConvertTo<opt_ty>(v);
      }

   template <class ty> requires adecc::is_in_type_list_v<ty, adecc::defined_values_types>
   ty get(std::size_t const iRow, std::size_t const iCol) const {
      auto o = get<std::optional<ty>>(iRow, iCol);
      if(!o) {
         throw std::runtime_error{"get: empty cell but non-optional requested"};
         }
      return *o;
      }

   // ------------------------------------------------------------------
   // SET: set<ty>(...) und set<std::optional<ty>>(...)
   // ty muss in defined_param_types liegen
   // ------------------------------------------------------------------
   template <class ty> requires adecc::is_in_type_list_v<ty, adecc::defined_param_types>
   void set(std::size_t const iRow, std::size_t const iCol, ty const& v) {
      set<std::optional<ty>>(iRow, iCol, std::optional<ty>{v});
      }

   template <class opt_ty>
   requires (
      adecc::is_optional_v<opt_ty> &&
      adecc::is_in_type_list_v<typename opt_ty::value_type, adecc::defined_param_types>
      )
   void set(std::size_t const iRow, std::size_t const iCol, opt_ty const& ov) {
      using T = typename opt_ty::value_type;

      if(iRow >= rows())    throw std::out_of_range{"set: row"};
      if(iCol >= columns()) throw std::out_of_range{"set: col"};

      pView_->DataController->Values[static_cast<int>(iRow)][static_cast<int>(iCol)] =
               ConvertTo<System::Variant>(ov);
      }

private:
   void Clear_() {
      pView_->DataController->RecordCount = 0;
      pView_->ClearItems();
      pView_->Bands->Clear();
      }

   void EnsureBands_(int const iCount) {
      int const iNeeded = iCount > 0 ? iCount : 1;

      while(pView_->Bands->Count < iNeeded) {
         TcxGridBand* pBand                 = pView_->Bands->Add();
         pBand->Caption                     = System::UnicodeString{};
         pBand->Options->HoldOwnColumnsOnly = true;
         }
      }

   static TAlignment MapAlignment_(EAlignmentType const eAlignment) {
      switch(eAlignment) {
         case EAlignmentType::right:   return taRightJustify;
         case EAlignmentType::center:  return taCenter;
         case EAlignmentType::left:    [[fallthrough]];
         case EAlignmentType::unknown: [[fallthrough]];
         default:                      return taLeftJustify;
         }
      }

   void ApplyBands_() {
      bool boVisible = vecBands_.size() > 1;
      pView_->OptionsView->BandHeaders = boVisible;
      pView_->OptionsCustomize->BandsQuickCustomization = boVisible;

      EnsureBands_(vecBands_.empty() ? 1 : static_cast<int>(vecBands_.size()));
      
      for(int i = 0; i < pView_->Bands->Count; ++i) {
         TcxGridBand* pBand = pView_->Bands->Items[i];
         if(!pBand) {
            continue;
            }

         pBand->Options->HoldOwnColumnsOnly = true;

         if(static_cast<std::size_t>(i) < vecBands_.size()) {
            auto const& [strCaption, bVisible] = vecBands_[static_cast<std::size_t>(i)];
            pBand->Caption = strCaption;
            pBand->Visible = bVisible;
            }
         else {
            pBand->Caption = System::UnicodeString{};
            pBand->Visible = false;
            }
         }
      }

   static std::pair<TcxSummaryKind, std::string> MapAggregate_(EGridColAggregate const eAggregate) {
      switch(eAggregate) {
         case EGridColAggregate::none:           return { skNone,    "" };
         case EGridColAggregate::distinct_count: [[fallthrough]]; // existiert leider nicht
         case EGridColAggregate::count:          return { skCount,   "Count" };
         case EGridColAggregate::average:        return { skAverage, "Sum" };
         case EGridColAggregate::min:            return { skMin,     "Min" };
         case EGridColAggregate::max:            return { skMax,     "Max" };
         case EGridColAggregate::sum:            return { skSum,     "Sum" };
         }
      }
      
   void ApplyAggregate_(::TcxGridBandedColumn* col,
                        EGridColAggregate const eAggregate,
                        TAlignment const eAlignment,
                        std::string const strFormat) {
      auto [kind, text] = MapAggregate_(eAggregate);
      
      if(eAggregate != EGridColAggregate::none) {
         // normaler Footer
         col->Summary->FooterKind        = kind; 
         col->Summary->FooterFormat      = strFormat.c_str();
         col->FooterAlignmentHorz        = eAlignment;

         // Footer wenn Group expanded
         col->Summary->GroupFooterKind   = kind;
         col->Summary->GroupFooterFormat = strFormat.c_str();

         // Anzeige in der Gruppierungszeile, erstmal ausgeschaltet (hat Aussetzer)
         col->Summary->GroupKind         = skNone;
         /*
         col->Summary->GroupKind         = kind;
         col->Summary->GroupFormat       = std::format("{}({}) = {}",
                                                   text,
                                                   ConvertTo<std::string>(col->Caption),
                                                   strFormat.size() > 0 ? strFormat : "0").c_str();
         */
         }
      else {
         col->Summary->FooterKind      = skNone;
         col->Summary->GroupFooterKind = skNone;
         col->Summary->GroupKind       = skNone;
         }
      
      }

   // ------------------------------------------------------------------------------
   void ApplyColumn_(::TcxGridBandedColumn* col,
                     TAlignment const eAlignment, 
                     EGridColType const eType, int const iScale,
                     EGridColAggregate const eAggregate,
                     std::string const& strRepository = "") {
      auto BuildFormat = [](int iDecimals, std::string&& add_on) -> std::string {
         return std::format(",0.{0:0>{1}}{2};-,0.{0:0>{1}}{2}", "", iDecimals, add_on);
         };
         
      std::string strFormat = {};
      col->HeaderAlignmentHorz = eAlignment;
         
      switch(eType) {
         case EGridColType::text:
            col->PropertiesClass = __classid(TcxTextEditProperties);
            break;

         case EGridColType::memo:
            col->PropertiesClass = __classid(TcxMemoProperties);
            break;

         case EGridColType::integer: {
            col->DataBinding->ValueTypeClass  = __classid(TcxIntegerValueType);
            col->PropertiesClass              = __classid(TcxCalcEditProperties);
            strFormat = "0";
            if(auto pProps = dynamic_cast<TcxCalcEditProperties*>(col->Properties); pProps) {
               pProps->Precision     = 0;
               pProps->DisplayFormat = strFormat.c_str(); 
               }
            }
            break;

         case EGridColType::floating: {
            col->DataBinding->ValueTypeClass  = __classid(TcxFloatValueType);
            col->PropertiesClass              = __classid(TcxCalcEditProperties);
            strFormat = BuildFormat(iScale, "");
            if(auto pProps = dynamic_cast<TcxCalcEditProperties*>(col->Properties); pProps) {
               pProps->Precision     = iScale; 
               pProps->DisplayFormat = strFormat.c_str();
               }
            }
            break;

         case EGridColType::currency: {
            col->DataBinding->ValueTypeClass  = __classid(TcxCurrencyValueType);
            col->PropertiesClass              = __classid(TcxCurrencyEditProperties);
            strFormat = BuildFormat(iScale, ""); // hier kann ein Währungssymbol stehen
            if(auto pProps  = dynamic_cast<TcxCurrencyEditProperties*>(col->Properties); pProps) {
               pProps->DecimalPlaces               = iScale; 
               pProps->DisplayFormat               = strFormat.c_str();
               pProps->UseDisplayFormatWhenEditing = true;
               }
            }
            break;

         case EGridColType::percent: {
            col->DataBinding->ValueTypeClass  = __classid(TcxFloatValueType);
            col->PropertiesClass              = __classid(TcxCalcEditProperties);
            strFormat = BuildFormat(iScale, " %");
            if(auto pProps  = dynamic_cast<TcxCalcEditProperties*>(col->Properties); pProps) {
               pProps->Precision                   = iScale; 
               pProps->DisplayFormat               = strFormat.c_str();
               pProps->Alignment->Horz             = eAlignment;
               pProps->UseLeftAlignmentOnEditing   = false;               
               }
            }
            break;

         case EGridColType::count: {
            col->DataBinding->ValueTypeClass  = __classid(TcxIntegerValueType);
            col->PropertiesClass = __classid(TcxSpinEditProperties);
            strFormat = "0";
            if(auto pProps  = dynamic_cast<TcxSpinEditProperties*>(col->Properties); pProps) {
               pProps->ValueType                 = vtInt; 
               pProps->DisplayFormat             = strFormat.c_str();
               pProps->Alignment->Horz           = eAlignment;
               pProps->UseLeftAlignmentOnEditing = false;
               }
            }
            break;
            
         case EGridColType::checkbox:
            col->DataBinding->ValueTypeClass = __classid(TcxBooleanValueType);
            col->PropertiesClass             = __classid(TcxCheckBoxProperties);

            if(auto pProps  = dynamic_cast<TcxCheckBoxProperties*>(col->Properties); pProps) {
               pProps->ValueChecked   = true;
               pProps->ValueUnchecked = false;
               pProps->AllowGrayed    = false;
               }            
            break;

         case EGridColType::combobox: {
            col->DataBinding->ValueTypeClass = __classid(TcxIntegerValueType);
            col->PropertiesClass             = __classid(TcxComboBoxProperties);
            
            auto it = mpImageComboRepositoryItems_.find(strRepository);
            if(it == mpImageComboRepositoryItems_.end() || !it->second) {
               throw std::runtime_error{
                         std::format("ApplyColumn_: repository item '{}' not found", strRepository)
                        };
               }
            else {
               col->RepositoryItem = it->second;
               }
            }
            break;
         
         case EGridColType::boolean:
            col->DataBinding->ValueTypeClass = __classid(TcxBooleanValueType);
            col->PropertiesClass             = __classid(TcxTextEditProperties);
            break;

         case EGridColType::date: {
            col->DataBinding->ValueTypeClass = __classid(TcxDateTimeValueType);
            using prop_ty = TcxDateEditProperties;
            col->PropertiesClass = __classid(prop_ty);
            if(auto pProps = dynamic_cast<prop_ty*>(col->GetProperties()); pProps) {
               pProps->Kind     = ckDate;
               pProps->Alignment->Horz = eAlignment;
               pProps->UseLeftAlignmentOnEditing = false;
               pProps->ShowTime = false;
               pProps->SaveTime = false;
               } 
            }
            break;

         case EGridColType::datetime: {
            col->DataBinding->ValueTypeClass = __classid(TcxDateTimeValueType);
            using prop_ty = TcxDateEditProperties;
            col->PropertiesClass = __classid(prop_ty);
            if(auto pProps = dynamic_cast<prop_ty*>(col->GetProperties()); pProps) {
               pProps->Kind     = ckDateTime;
               pProps->ShowTime = true;
               pProps->SaveTime = true;
               pProps->Alignment->Horz = eAlignment;
               pProps->UseLeftAlignmentOnEditing = false;
               }
            }
            break;

         case EGridColType::time:
            col->PropertiesClass = __classid(TcxTimeEditProperties);
            break;
         }
         
      if(TcxCustomEditProperties* pProps = col->GetProperties(); pProps) {
         pProps->Alignment->Horz = eAlignment;
         }

      ApplyAggregate_(col, eAggregate, eAlignment, strFormat);
      }


    template <adecc::StreamPolicy SP>
	void SetBands_(vecBandCaptions<SP> const& vecBands) {
       vecBands_.clear();
       vecBands_.reserve(vecBands.size());

       for(auto const& t : vecBands) {
          vecBands_.emplace_back(
                adecc::ConvertTo<System::UnicodeString>(std::get<0>(t)), std::get<1>(t)
                );
          }
       }

   [[nodiscard]] int ScaleColumnWidth_(int const iWidth) const noexcept {
      int const iEffectiveWidth = (std::max)(0, iWidth);

      if(iEffectiveWidth == 0) {
         return 0;
         }

      int const iWidth96Dpi =
         iEffectiveWidth * iColumnWidthScalePx_ + iColumnWidthPaddingPx_;

      return ScaleWidthByPpi(pView_, iWidth96Dpi);
      }

   static int ScaleWidthByPpi(::TcxGridBandedTableView* pView, int const iWidth96Dpi) {
      if(!pView) {
         return iWidth96Dpi;
         }

      TCustomForm* pForm = GetParentForm(pView->Site);
      int const iPpi = pForm ? pForm->CurrentPPI : Screen->PixelsPerInch;
      return MulDiv(iWidth96Dpi, iPpi, 96);
      }
      


   void bind_image_combo_repository(::TcxGridBandedColumn* pCol, std::string const& strName) {
      if(!pCol) {
         throw std::invalid_argument{"bind_image_combo_repository: null column"};
         }

      auto it = mpImageComboRepositoryItems_.find(strName);
      if(auto it = mpImageComboRepositoryItems_.find(strName); it == mpImageComboRepositoryItems_.end()) {
         throw std::out_of_range{"bind_image_combo_repository: repository item not found"};
         }
      else {
         pCol->RepositoryItem = it->second;
         }
      }

   void DestroyRepository_() {
      mpImageComboRepositoryItems_.clear();

      if(pEditRepository_) {
         auto pTmp = pEditRepository_;
         pEditRepository_ = nullptr;
         pTmp->Free();
         }
      }
 
   template <adecc::Integral ty = int, adecc::StreamPolicy SP = adecc::AnsiStreamPolicy>
   void CreateRepository_(tplImageComboRepository<ty, SP> const& aRepository) {

      if(!pView_ || !pView_->Owner) {
         throw std::runtime_error{"CreateRepository_: invalid owner"};
         }
      DestroyRepository_();
      auto const& [repo_name, vecRepoItems] = aRepository;

      pEditRepository_ = new TcxEditRepository(pView_->Owner);
      pEditRepository_->Name = ConvertTo<System::UnicodeString>(repo_name);
      
      // mpImageComboRepositoryItems_.clear();

      for(auto const& [strItemName, vecValues] : vecRepoItems) {
         auto* pRepoItem = new TcxEditRepositoryImageComboBoxItem(pEditRepository_);
         auto* pProps = pRepoItem->Properties;
         if(!pProps) {
			throw std::runtime_error{"CreateRepository_: properties null"};
            }
                     
         pProps->Items->BeginUpdate();
         auto scope = adecc::makeScopeExit([&]() { pProps->Items->EndUpdate(); });

         pProps->Items->Clear();

         for(auto const& [aValue, strText] : vecValues) {
            auto* pComboItem = pProps->Items->Add();
            pComboItem->Value = static_cast<int>(aValue);
            pComboItem->Description = adecc::ConvertTo<System::UnicodeString>(strText);
            }

         pProps->Items->EndUpdate();
         mpImageComboRepositoryItems_.emplace(strItemName, pRepoItem);
         }
      }
         
      
private:
   ::TcxGridBandedTableView* pView_ = nullptr;
   std::vector<std::tuple<System::UnicodeString, bool>> vecBands_ {};

   int iColumnWidthScalePx_ = 8;
   int iColumnWidthPaddingPx_ = 0;

   ::TcxEditRepository* pEditRepository_ = nullptr;
   std::unordered_map<std::string, ::TcxEditRepositoryImageComboBoxItem*> mpImageComboRepositoryItems_ {};
};

static_assert(adecc::grid::table_type<BandedTableViewBackend>,
              "Concept table_type isn't fulfilled for BandedTableViewBackend");

} // end of namespace adecc::delphi::vcl::devexpress

#endif
