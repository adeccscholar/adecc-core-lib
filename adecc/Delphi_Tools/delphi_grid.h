#pragma once

#include "grid_wrapper.h"
#include "stream_tools.h"
#include "convert_core.h"
#include "delphi_convert.h"

#include "delphi_waitcursor.h"

#include <algorithm>
#include <cmath>
#include <print> // für kontrolle

// ============================================================================
// Physische Backends (Adapter): VCL / FMX — erfüllen das Concept
// ============================================================================

namespace adecc::delphi {


inline int ScaleWidthByPpi(int const iWidth96Dpi, int const iPpi) {
   return MulDiv(iWidth96Dpi, iPpi, 96);
   }

}

#if defined(BUILD_WITH_FMX)
   #include <FMX.Grid.hpp>

namespace adecc::delphi::fmx {

inline int GetCurrentPpi(Fmx::Controls::TControl* pControl) {
   if(!pControl) {
      return 96;
      }

   float const flScale = pControl->AbsoluteScale.X;
   if(flScale <= 0.0f) {
	  return 96;
	  }

   return static_cast<int>(std::lround(96.0f * flScale));
}

inline int ScaleWidthByPpi(Fmx::Controls::TControl* pControl, int const iWidth96Dpi) {
   return adecc::delphi::ScaleWidthByPpi(iWidth96Dpi, GetCurrentPpi(pControl));
}

inline int ScaleWidthByPpi(Fmx::Grid::TStringGrid* pGrid, int const iWidth96Dpi) {
   return ScaleWidthByPpi(static_cast<Fmx::Controls::TControl*>(pGrid), iWidth96Dpi);
}


class StringGridBackend {
public:
   using size_type = std::size_t;

   StringGridBackend(::TStringGrid* pGrid) : pGrid_{pGrid} {
	  if(!pGrid_) throw std::invalid_argument{"delphi::fmx::StringGridBackend: null"};
	  else [[likely]] {
		 pGrid_->ReadOnly = true;
		 pGrid_->Options <<= TGridOption::RowSelect;
		 pGrid_->Options >>= TGridOption::ColumnMove;
		 pGrid_->Options <<= TGridOption::AlwaysShowSelection;
		 }
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

   template <adecc::StreamPolicy SP = adecc::AnsiStreamPolicy>
   void set_caption(adecc::vecCaptions<SP> const& caps, bool clear = true) {
	  pGrid_->BeginUpdate();
	  try {
		 if(clear) {
			pGrid_->ClearColumns();
			pGrid_->RowCount = 0;
			}

		 auto mapAlign = [](EAlignmentType a) -> TTextAlign {
			switch (a) {
			   case EAlignmentType::right:   return TTextAlign::Trailing;
			   case EAlignmentType::center:  return TTextAlign::Center;
			   case EAlignmentType::left:    [[fallthrough]];
			   case EAlignmentType::unknown: [[fallthrough]];
			   default:                      return TTextAlign::Leading;
			   }
			};

		 for (auto const& t : caps) {
			auto* col = new TStringColumn(pGrid_);
			col->Header = adecc::ConvertTo<System::UnicodeString>(std::get<0>(t));
			col->Width  = ScaleColumnWidth_(static_cast<int>(std::get<1>(t)));
			col->HorzAlign = mapAlign(std::get<2>(t));
			pGrid_->AddObject(col);
			}
		 pGrid_->EndUpdate();
		 }
	  catch(Exception const& ex) {
		 pGrid_->EndUpdate();
		 throw std::runtime_error(AnsiString(ex.Message).c_str());
		 }
	  catch (...) {
		 pGrid_->EndUpdate();
		 throw;
		 }
	  }


   template <class... Args>
   void set_caption(std::vector<std::tuple<Args...>> const& caps, bool clear = true) {
	  auto mapAlign = [](auto const& a) -> TTextAlign {
		 using A = std::remove_cvref_t<decltype(a)>;
		 if constexpr (std::is_same_v<A, EAlignmentType>) {
			switch (a) {
			   case EAlignmentType::right:   return TTextAlign::Trailing;
			   case EAlignmentType::center: return TTextAlign::Center;
			   case EAlignmentType::left:    [[fallthrough]];
			   case EAlignmentType::unknown: [[fallthrough]];
			   default:                      return TTextAlign::Leading;
			   }
			}
		 else if constexpr (std::is_integral_v<A>) {
			switch (static_cast<int>(a)) {
			   case 2:  return TTextAlign::Trailing; // right
			   case 3:  return TTextAlign::Center;   // center
			   case 1:  [[fallthrough]];
			   default: return TTextAlign::Leading;  // left/unknown
			   }
			}
		 else {
			return TTextAlign::Leading;
			}
		 };

	  pGrid_->BeginUpdate();
	  try {
		 if (clear) {
			pGrid_->ClearColumns();
			pGrid_->RowCount = 0;
			}

		 for (auto const& t : caps) {
			auto* col = new TStringColumn(pGrid_);
			using H = std::remove_cvref_t<std::tuple_element_t<0, std::tuple<Args...>>>;
			col->Header = adecc::ConvertTo<System::UnicodeString>(std::get<0>(t));

			if constexpr (sizeof...(Args) >= 2) {
			   using W = std::remove_cvref_t<std::tuple_element_t<1, std::tuple<Args...>>>;
			   if constexpr (std::is_integral_v<W>)
				  col->Width = ScaleColumnWidth_(static_cast<int>(std::get<1>(t)));
			   }
			if constexpr (sizeof...(Args) >= 3) {
			   col->HorzAlign = mapAlign(std::get<2>(t));
			   }
			pGrid_->AddObject(col);
			}
		 pGrid_->EndUpdate();
		 }
	  catch(Exception const& ex) {
		 pGrid_->EndUpdate();
		 throw std::runtime_error(AnsiString(ex.Message).c_str());
		 }
	  catch (...) {
		 pGrid_->EndUpdate();
		 throw;
		 }
	  }

   void reset(bool const bResetRepository = true) {
	  pGrid_->BeginUpdate();
	  try {
		 pGrid_->RowCount = 0;
		 if(bResetRepository) {
			pGrid_->ClearColumns();
			}
		 pGrid_->EndUpdate();
		 }
	  catch(Exception const& ex) {
		 pGrid_->EndUpdate();
		 throw std::runtime_error(AnsiString(ex.Message).c_str());
		 }
	  catch(...) {
		 pGrid_->EndUpdate();
		 throw;
		 }
	  }




   [[nodiscard]] bool freeze() {
	  if(!pGrid_) {
		 return false;
		 }

	  pGrid_->BeginUpdate();
	  return true;
	  }

   void unfreeze(bool const bFrozen) {
	  if(bFrozen && pGrid_) {
		 pGrid_->EndUpdate();
		 }
	  }

   [[nodiscard]] auto make_wait_cursor() {
	  return adecc::WaitCursor {};
	  }

   size_type rows() const noexcept    { return (size_type)pGrid_->RowCount; }
   size_type columns() const noexcept { return (size_type)pGrid_->ColumnCount; }


   [[nodiscard]] size_type current_row() const noexcept {
	  return static_cast<size_type>(pGrid_->Row);
	  }

   [[nodiscard]] size_type current_column() const noexcept {
	  return static_cast<size_type>(pGrid_->Col);
	  }

   bool focus_row(size_t iRow) {
	  if(iRow >= rows()) return false;
	  pGrid_->Row = static_cast<int>(iRow);
	  return true;
	  }

   size_type insert_row() {
	  int const phys = pGrid_->RowCount;
	  pGrid_->RowCount = phys + 1;
	  return (size_type)phys;         // 0-basiert, keine Headerzeile
	  }

   template <class opt_ty>
		  requires ( adecc::is_optional_v<opt_ty> &&
					 adecc::is_in_type_list_v<typename opt_ty::value_type, adecc::defined_values_types> )
   opt_ty get(std::size_t const iRow, std::size_t const iCol) const {
	  using T = typename opt_ty::value_type;
	  int const r = static_cast<int>(iRow);
	  int const c = static_cast<int>(iCol);
	  if (r < 0 || r >= pGrid_->RowCount)     throw std::out_of_range{"get: row"};
	  if (c < 0 || c >= pGrid_->ColumnCount)  throw std::out_of_range{"get: col"};

	  System::UnicodeString const s = pGrid_->Cells[c][r];
	  if (s.IsEmpty()) return opt_ty{}; // nullopt

	  if (auto o = adecc::ConvertToOpt<T>(s); o) return o;   // nutzt deine Optional-Konvertierung
	  return opt_ty{ adecc::ConvertTo<T>(s) };               // Fallback über ConvertTo
	  }

   template <class ty>
	   requires adecc::is_in_type_list_v<ty, adecc::defined_values_types>
   ty get(std::size_t const iRow, std::size_t const iCol) const {
	  auto o = get<std::optional<ty>>(iRow, iCol);
	  if(!o) throw std::runtime_error{"get: empty cell but non-optional requested"};
	  return *o;
	  }


/*

   // (optional) Bequemlichkeit: nicht-optional, wirft bei leer
   template <class ty>
   requires adecc::is_in_type_list_v<ty, adecc::defined_values_types>
   ty get(std::size_t const iRow, std::size_t const iCol) const {
	  auto o = get<std::optional<ty>>(iRow, iCol);
	  if(!o) throw std::runtime_error{"get: empty cell but non-optional requested"};
	  return *o;
   }

*/




   template <class ty>
		  requires adecc::is_in_type_list_v<ty, adecc::defined_param_types>
   void set(std::size_t const iRow, std::size_t const iCol, ty const& v) {
	  int const r = static_cast<int>(iRow);
	  int const c = static_cast<int>(iCol);
	  if (r < 0 || r >= pGrid_->RowCount)     throw std::out_of_range{"set: row"};
	  if (c < 0 || c >= pGrid_->ColumnCount)  throw std::out_of_range{"set: col"};

	  pGrid_->Cells[c][r] = adecc::ConvertTo<System::UnicodeString>(v);
	  }

   template <class opt_ty>
		  requires ( adecc::is_optional_v<opt_ty> &&
					 adecc::is_in_type_list_v<typename opt_ty::value_type, adecc::defined_param_types> )
   void set(std::size_t const iRow, std::size_t const iCol, opt_ty const& ov) {
	  using T = typename opt_ty::value_type;
	  int const r = static_cast<int>(iRow);
	  int const c = static_cast<int>(iCol);
	  if (r < 0 || r >= pGrid_->RowCount)     throw std::out_of_range{"set: row"};
	  if (c < 0 || c >= pGrid_->ColumnCount)  throw std::out_of_range{"set: col"};

	  if (ov) pGrid_->Cells[c][r] = adecc::ConvertTo<System::UnicodeString>(*ov);
	  else    pGrid_->Cells[c][r] = System::UnicodeString{};
	  }


private:
   [[nodiscard]] int ScaleColumnWidth_(int const iWidth) const {
      int const iEffectiveWidth = (std::max)(0, iWidth);

      if(iEffectiveWidth == 0) {
         return 0;
         }

      int const iWidth96Dpi =
         iEffectiveWidth * iColumnWidthScalePx_ + iColumnWidthPaddingPx_;

      return ScaleWidthByPpi(pGrid_, iWidth96Dpi);
      }

private:
   ::TStringGrid* pGrid_ = nullptr;
   int iColumnWidthScalePx_ = 8;
   int iColumnWidthPaddingPx_ = 0;
};


static_assert(adecc::grid::table_type<StringGridBackend>, "Concept table_type is't fulfilled for StringGridBackend");

} // end of namespace adecc::delphi::fmx

#endif

#if defined(BUILD_WITH_VCL)

#include <Vcl.ComCtrls.hpp>

namespace adecc::delphi::vcl {

inline int GetCurrentPpi(Vcl::Controls::TControl* pControl) {
   if(!pControl) {
	  return Screen->PixelsPerInch;
	  }

   Vcl::Forms::TCustomForm* pForm = Vcl::Forms::GetParentForm(pControl);

   return pForm ? pForm->CurrentPPI : Screen->PixelsPerInch;
   }

inline int ScaleWidthByPpi(Vcl::Controls::TControl* pControl, int const iWidth96Dpi) {
   return adecc::delphi::ScaleWidthByPpi(iWidth96Dpi, GetCurrentPpi(pControl));
   }

inline int ScaleWidthByPpi(Vcl::Comctrls::TListView* pListView, int const iWidth96Dpi) {
   return ScaleWidthByPpi(static_cast<Vcl::Controls::TControl*>(pListView), iWidth96Dpi);
   }


/*!
 \brief VCL-Adapter für TListView (Report-Ansicht)
 \details Spalte 0 = Caption, Spalte n>=1 = SubItems[n-1].
		  get/set arbeiten textbasiert über Deine Konvertierungsschicht.
*/
class ListViewBackend {
public:
   using size_type = std::size_t;

   explicit ListViewBackend(::TListView* pView) : pView_{pView} {
	  if(!pView_) throw std::invalid_argument{"delphi::vcl::ListViewBackend: null"};
	  pView_->ViewStyle   = vsReport;
	  pView_->RowSelect   = true;
	  pView_->ReadOnly    = true;    // kein editieren der Caption
	  pView_->MultiSelect = false;   // erst einmal eingeschränkt
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
   // Caption setzen: Headertext + optionale Breite (+ optionale Ausrichtung)
   // tuple<Hdr[, Width[, Align]]> — Align optional als int (0=Left,1=Right,2=Center)
   // ------------------------------------------------------------------
   // VCL: adecc::delphi::vcl::ListViewBackend
   template <adecc::StreamPolicy SP = adecc::AnsiStreamPolicy>
   void set_caption(adecc::vecCaptions<SP> const& caps, bool bClear = true) {
	  pView_->Columns->BeginUpdate();
	  try {
		 if(bClear) {
			pView_->Items->Clear();
			pView_->Columns->Clear();
			}

		 for (auto const& t : caps) {
			TListColumn* col = pView_->Columns->Add();
			// Header-Text aus Policy-String in UnicodeString konvertieren
			col->Caption   = adecc::ConvertTo<System::UnicodeString>(std::get<0>(t));
			// Spaltenbreite
			col->Width     = ScaleColumnWidth_(static_cast<int>(std::get<1>(t)));
			// Alignment abbilden
			switch (std::get<2>(t)) {
			   case EAlignmentType::right:   col->Alignment = taRightJustify; break;
			   case EAlignmentType::center:  col->Alignment = taCenter;       break;
			   case EAlignmentType::left:    [[fallthrough]];
			   case EAlignmentType::unknown: [[fallthrough]];
			   default:                      col->Alignment = taLeftJustify;  break;
			   }
			}
		 pView_->Columns->EndUpdate();
		 //ForceColumnAlignments(pView_);
		 }
	  catch(Exception const& ex) {
		 pView_->Columns->EndUpdate();
		 throw std::runtime_error(AnsiString(ex.Message).c_str());
		 }
	  catch (...) {
		 pView_->Columns->EndUpdate();
		 throw;
		 }
	  }


   template <class... Args>
   void set_caption(std::vector<std::tuple<Args...>> const& caps, bool clear = true) {
	  auto mapAlign = [](auto const& a) -> TAlignment {
		 using A = std::remove_cvref_t<decltype(a)>;
		 if constexpr (std::is_same_v<A, EAlignmentType>) {
			switch (a) {
			   case EAlignmentType::right:   return taRightJustify;
			   case EAlignmentType::center:  return taCenter;
			   case EAlignmentType::left:    [[fallthrough]];
			   case EAlignmentType::unknown: [[fallthrough]];
			   default:                      return taLeftJustify;
			   }
			}
		 else if constexpr (std::is_integral_v<A>) {
			switch (static_cast<int>(a)) {
			   case 2:  return taRightJustify; // right
			   case 3:  return taCenter;       // center
			   case 1:  [[fallthrough]];
			   default: return taLeftJustify;  // left/unknown
			   }
			}
		 else {
			return taLeftJustify;
			}
		 };

	  pView_->Columns->BeginUpdate();
	  try {
		 if (clear) {
			pView_->Items->Clear();
			pView_->Columns->Clear();
			}
		 for (auto const& t : caps) {
			TListColumn* col = pView_->Columns->Add();

			using H = std::remove_cvref_t<std::tuple_element_t<0, std::tuple<Args...>>>;
			col->Caption = adecc::ConvertTo<System::UnicodeString>(std::get<0>(t));

			if constexpr (sizeof...(Args) >= 2) {
			   using W = std::remove_cvref_t<std::tuple_element_t<1, std::tuple<Args...>>>;
			   if constexpr (std::is_integral_v<W>)
				  col->Width = ScaleColumnWidth_(static_cast<int>(std::get<1>(t)));
			   }
			if constexpr (sizeof...(Args) >= 3) {
			   col->Alignment = mapAlign(std::get<2>(t));
			   }
		   // std::println(std::cerr, "{:>40} {:>5} {}", AnsiString(col->Caption).c_str(), col->Width, static_cast<int>(col->Alignment));

			}
		pView_->Columns->EndUpdate();
		 //ForceColumnAlignments(pView_);
		 }
	  catch(Exception const& ex) {
		 pView_->Columns->EndUpdate();
		 throw std::runtime_error(AnsiString(ex.Message).c_str());
		 }
	  catch (...) {
		 pView_->Columns->EndUpdate();
		 throw;
		 }
	  }


   void reset(bool const bResetRepository = true) {
	  pView_->Columns->BeginUpdate();
	  try {
		 pView_->Items->Clear();
		 if(bResetRepository) {
			pView_->Columns->Clear();
			}
		 pView_->Columns->EndUpdate();
		 }
	  catch(Exception const& ex) {
		 pView_->Columns->EndUpdate();
		 throw std::runtime_error(AnsiString(ex.Message).c_str());
		 }
	  catch(...) {
		 pView_->Columns->EndUpdate();
		 throw;
		 }
	  }

   [[nodiscard]] bool freeze() {
	  if(!pView_) {
		 return false;
		 }

	  pView_->Items->BeginUpdate();
	  pView_->Columns->BeginUpdate();
	  return true;
	  }

   void unfreeze(bool const bFrozen) {
	  if(bFrozen && pView_) {
		 pView_->Columns->EndUpdate();
		 pView_->Items->EndUpdate();
		 }
	  }

   [[nodiscard]] auto make_wait_cursor() {
	  return adecc::WaitCursor {};
	  }


   static void ForceColumnAlignments(::TListView* lv) {
	  if (!lv || !lv->HandleAllocated()) return;

	  for (int i = 0; i < lv->Columns->Count; ++i) {
		 TAlignment const a = lv->Columns->Items[i]->Alignment;

		 UINT fmt = LVCFMT_LEFT;
		 switch (a) {
			case taRightJustify: fmt = LVCFMT_RIGHT;  break;
			case taCenter:       fmt = LVCFMT_CENTER; break;
			default:             fmt = LVCFMT_LEFT;   break;
			}

		 LVCOLUMN col{};
		 col.mask = LVCF_FMT;
		 col.fmt  = fmt;
		 ListView_SetColumn(lv->Handle, i, &col);
		 }
	  }

   // ------------------------------------------------------------------
   // Dimensionen / aktuelle Position
   // ------------------------------------------------------------------
   size_type rows()    const noexcept { return (size_type)pView_->Items->Count; }
   size_type columns() const noexcept { return (size_type)pView_->Columns->Count; }

   [[nodiscard]] size_type current_row() const noexcept {
	  int const idx = pView_->ItemIndex;        // -1 wenn keine Auswahl
	  return idx >= 0 ? (size_type)idx : (size_type)0;
	  }

   [[nodiscard]] size_type current_column() const noexcept {
	  // TListView hat keine aktive Spalte; 0 als definierter Rückgabewert
	  return (size_type)0;
	  }

   bool focus_row(size_t iRow) {
	  if(iRow >= rows()) return false;

	  int const iItemIndex = static_cast<int>(iRow);
	  pView_->ItemIndex = iItemIndex;

	  if(iItemIndex >= 0 && iItemIndex < pView_->Items->Count) {
		 pView_->Items->Item[iItemIndex]->Focused  = true;
		 pView_->Items->Item[iItemIndex]->Selected = true;
		 pView_->Items->Item[iItemIndex]->MakeVisible(true);
		 }
	  return true;
	  }

   // Neue Zeile anhängen, Index zurückgeben
   size_type insert_row() {
      TListItem* it = pView_->Items->Add();
      it->Caption = System::UnicodeString{};
      return (size_type)(it->Index);
   }

   // ------------------------------------------------------------------
   // GET: geforderte Signatur — get<std::optional<T>>(row,col) -> std::optional<T>
   //     T muss in defined_values_types liegen
   // ------------------------------------------------------------------
  template<class opt_ty>
requires (adecc::is_optional_v<opt_ty> &&
          adecc::is_in_type_list_v<typename opt_ty::value_type, adecc::defined_values_types>)
opt_ty get(std::size_t iRow, std::size_t iCol) const {
   using T = typename opt_ty::value_type;

   std::size_t const nrows = static_cast<std::size_t>(pView_->Items->Count);
   std::size_t const ncols = static_cast<std::size_t>(pView_->Columns->Count);
   if (iRow >= nrows) throw std::out_of_range{"get: row"};
   if (iCol >= ncols) throw std::out_of_range{"get: col"};

   // erste Spalte = Caption, ab 1: SubItems[0..]
   UnicodeString s;
   if (iCol == 0) s = pView_->Items->Item[static_cast<int>(iRow)]->Caption;
   else           s = pView_->Items->Item[static_cast<int>(iRow)]->SubItems->Strings[static_cast<int>(iCol-1)];

   if (s.IsEmpty()) return opt_ty{};
   if (auto o = adecc::ConvertToOpt<T>(s); o) return o;
   return opt_ty{ adecc::ConvertTo<T>(s) };
}

   // (optional) Bequemlichkeit: nicht-optional, wirft bei leer
   template <class ty>
   requires adecc::is_in_type_list_v<ty, adecc::defined_values_types>
   ty get(std::size_t const iRow, std::size_t const iCol) const {
      auto o = get<std::optional<ty>>(iRow, iCol);
      if(!o) throw std::runtime_error{"get: empty cell but non-optional requested"};
      return *o;
   }

   // ------------------------------------------------------------------
   // SET: set<ty>(...) und set<std::optional<ty>>(...), T ? defined_param_types
   // ------------------------------------------------------------------
   template <class ty>
      requires adecc::is_in_type_list_v<ty, adecc::defined_param_types>
   void set(std::size_t const iRow, std::size_t const iCol, ty const& v) {
      set<std::optional<ty>>(iRow, iCol, std::optional<ty>{v});
      }

   template <class opt_ty>
      requires ( adecc::is_optional_v<opt_ty> &&
                 adecc::is_in_type_list_v<typename opt_ty::value_type, adecc::defined_param_types> )
   void set(std::size_t const iRow, std::size_t const iCol, opt_ty const& ov) {
      using T = typename opt_ty::value_type;

      if(iRow >= rows())    throw std::out_of_range{"set: row"};
      if(iCol >= columns()) throw std::out_of_range{"set: col"};

      TListItem* it = pView_->Items->Item[(int)iRow];
      System::UnicodeString us = ov ? adecc::ConvertTo<System::UnicodeString>(*ov)
                                    : System::UnicodeString{};

      if(iCol == 0) {
         it->Caption = us;
         }
      else {
         // SubItems ggf. bis Zielindex auffüllen
         int const target = (int)iCol - 1;
         while(it->SubItems->Count <= target) {
            it->SubItems->Add(System::UnicodeString{});
            }
         it->SubItems->Strings[target] = us;
         }
      }

private:
   [[nodiscard]] int ScaleColumnWidth_(int const iWidth) const {
      int const iEffectiveWidth = (std::max)(0, iWidth);

      if(iEffectiveWidth == 0) {
         return 0;
         }

      int const iWidth96Dpi =
         iEffectiveWidth * iColumnWidthScalePx_ + iColumnWidthPaddingPx_;

      return ScaleWidthByPpi(pView_, iWidth96Dpi);
      }

private:
   ::TListView* pView_ = nullptr;
   int iColumnWidthScalePx_ = 8;
   int iColumnWidthPaddingPx_ = 0;
};

static_assert(adecc::grid::table_type<ListViewBackend>,
              "Concept table_type isn't fulfilled for ListViewBackend");


} // namespace adecc::delphi::vcl

namespace std {

template<class... Ts>
struct indirectly_readable_traits<
   adecc::grid::GridModel<adecc::delphi::vcl::ListViewBackend>::RowRefIter<Ts...>
   > {
   using value_type = std::tuple<Ts...>;
   using reference  = adecc::grid::GridModel<adecc::delphi::vcl::ListViewBackend>
                        ::RowRefProxy<Ts...>&;
   // optional: using differencetype = std::ptrdiff_t;
};

}

#endif


