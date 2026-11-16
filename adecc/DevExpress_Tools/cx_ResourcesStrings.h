#pragma once

#include <vcl.h>
#pragma hdrstop

#include "delphi_convert.h"

#include <System.SysUtils.hpp>
#include <System.Classes.hpp>

#include <cxClasses.hpp>
#include <dxCore.hpp>

#include <cxGridStrs.hpp>
#include <cxGridPopupMenuConsts.hpp>

#include <fstream>
#include <vector>

#include <print>

namespace adecc::delphi::vcl::devexpress {

struct TResourceEntry {
      std::string_view  szIdentifier;
      TcxResourceStringID pResource;
   };

[[nodiscard]] std::string EscapeCppStringLiteral(std::string const& strInput) {
   static std::vector<std::pair<char, char const*>> const vecReplacements {
         { '\\', "\\\\" },
         { '\"', "\\\"" },
         { '\n', "\\n" },
         { '\r', "\\r" },
         { '\t', "\\t" },
         { '\b', "\\b" },
         { '\f', "\\f" },
         { '\v', "\\v" },
         { '\a', "\\a" }
      };

   std::string strResult;
   strResult.reserve(strInput.size() + 8);

   strResult.push_back('"');

   for (unsigned char const chValue : strInput) {

      auto const it = std::find_if(
           vecReplacements.begin(),
           vecReplacements.end(),
           [chValue](auto const& aPair) {
              return aPair.first == chValue;
              }
           );

      if (it != vecReplacements.end()) {
         strResult += it->second;
         }
      else if (chValue < 0x20) {
         char szBuffer[5];
         std::snprintf(szBuffer, sizeof(szBuffer), "\\x%02X", chValue);
         strResult += szBuffer;
         }
      else {
         strResult.push_back(static_cast<char>(chValue));
         }
      }

   strResult.push_back('"');

   return strResult;
   }
   
std::vector<TResourceEntry> CreateGridResourceList() {
      return {
         { "Cxgridstrs::_scxGridRecursiveLevels", &Cxgridstrs::_scxGridRecursiveLevels },
         { "Cxgridstrs::_scxGridDeletingFocusedConfirmationText", &Cxgridstrs::_scxGridDeletingFocusedConfirmationText },
         { "Cxgridstrs::_scxGridDeletingSelectedConfirmationText", &Cxgridstrs::_scxGridDeletingSelectedConfirmationText },
         { "Cxgridstrs::_scxGridNoDataInfoText", &Cxgridstrs::_scxGridNoDataInfoText },
         { "Cxgridstrs::_scxGridFilterRowInfoText", &Cxgridstrs::_scxGridFilterRowInfoText },
         { "Cxgridstrs::_scxGridNewItemRowInfoText", &Cxgridstrs::_scxGridNewItemRowInfoText },
         { "Cxgridstrs::_scxGridFindPanelClearButtonCaption", &Cxgridstrs::_scxGridFindPanelClearButtonCaption },
         { "Cxgridstrs::_scxGridFindPanelFindButtonCaption", &Cxgridstrs::_scxGridFindPanelFindButtonCaption },
         { "Cxgridstrs::_scxGridFindPanelNextButtonCaption", &Cxgridstrs::_scxGridFindPanelNextButtonCaption },
         { "Cxgridstrs::_scxGridFindPanelPreviousButtonCaption", &Cxgridstrs::_scxGridFindPanelPreviousButtonCaption },
         { "Cxgridstrs::_scxGridFindPanelInfoText", &Cxgridstrs::_scxGridFindPanelInfoText },
         { "Cxgridstrs::_scxGridFilterIsEmpty", &Cxgridstrs::_scxGridFilterIsEmpty },
         { "Cxgridstrs::_scxGridCustomizationFormCaption", &Cxgridstrs::_scxGridCustomizationFormCaption },
         { "Cxgridstrs::_scxGridCustomizationFormColumnsPageCaption", &Cxgridstrs::_scxGridCustomizationFormColumnsPageCaption },
         { "Cxgridstrs::_scxGridGroupByBoxCaption", &Cxgridstrs::_scxGridGroupByBoxCaption },
         { "Cxgridstrs::_scxGridFilterApplyButtonCaption", &Cxgridstrs::_scxGridFilterApplyButtonCaption },
         { "Cxgridstrs::_scxGridFilterCustomizeButtonCaption", &Cxgridstrs::_scxGridFilterCustomizeButtonCaption },
         { "Cxgridstrs::_scxGridColumnsQuickCustomizationHint", &Cxgridstrs::_scxGridColumnsQuickCustomizationHint },
         { "Cxgridstrs::_scxGridCustomizationFormBandsPageCaption", &Cxgridstrs::_scxGridCustomizationFormBandsPageCaption },
         { "Cxgridstrs::_scxGridBandsQuickCustomizationHint", &Cxgridstrs::_scxGridBandsQuickCustomizationHint },
         { "Cxgridstrs::_scxGridCustomizationFormRowsPageCaption", &Cxgridstrs::_scxGridCustomizationFormRowsPageCaption },
         { "Cxgridstrs::_scxGridConverterIntermediaryMissing", &Cxgridstrs::_scxGridConverterIntermediaryMissing },
         { "Cxgridstrs::_scxGridConverterNotExistGrid", &Cxgridstrs::_scxGridConverterNotExistGrid },
         { "Cxgridstrs::_scxGridConverterNotExistComponent", &Cxgridstrs::_scxGridConverterNotExistComponent },
         { "Cxgridstrs::_scxImportErrorCaption", &Cxgridstrs::_scxImportErrorCaption },
         { "Cxgridstrs::_scxNotExistGridView", &Cxgridstrs::_scxNotExistGridView },
         { "Cxgridstrs::_cxSEditRepositoryExtLookupComboBoxItem", &Cxgridstrs::_cxSEditRepositoryExtLookupComboBoxItem },
         { "Cxgridstrs::_scxGridYesterday", &Cxgridstrs::_scxGridYesterday },
         { "Cxgridstrs::_scxGridToday", &Cxgridstrs::_scxGridToday },
         { "Cxgridstrs::_scxGridTomorrow", &Cxgridstrs::_scxGridTomorrow },
         { "Cxgridstrs::_scxGridLast30Days", &Cxgridstrs::_scxGridLast30Days },
         { "Cxgridstrs::_scxGridLast14Days", &Cxgridstrs::_scxGridLast14Days },
         { "Cxgridstrs::_scxGridLast7Days", &Cxgridstrs::_scxGridLast7Days },
         { "Cxgridstrs::_scxGridNext7Days", &Cxgridstrs::_scxGridNext7Days },
         { "Cxgridstrs::_scxGridNext14Days", &Cxgridstrs::_scxGridNext14Days },
         { "Cxgridstrs::_scxGridNext30Days", &Cxgridstrs::_scxGridNext30Days },
         { "Cxgridstrs::_scxGridLastTwoWeeks", &Cxgridstrs::_scxGridLastTwoWeeks },
         { "Cxgridstrs::_scxGridLastWeek", &Cxgridstrs::_scxGridLastWeek },
         { "Cxgridstrs::_scxGridThisWeek", &Cxgridstrs::_scxGridThisWeek },
         { "Cxgridstrs::_scxGridNextWeek", &Cxgridstrs::_scxGridNextWeek },
         { "Cxgridstrs::_scxGridNextTwoWeeks", &Cxgridstrs::_scxGridNextTwoWeeks },
         { "Cxgridstrs::_scxGridLastMonth", &Cxgridstrs::_scxGridLastMonth },
         { "Cxgridstrs::_scxGridThisMonth", &Cxgridstrs::_scxGridThisMonth },
         { "Cxgridstrs::_scxGridNextMonth", &Cxgridstrs::_scxGridNextMonth },
         { "Cxgridstrs::_scxGridLastYear", &Cxgridstrs::_scxGridLastYear },
         { "Cxgridstrs::_scxGridThisYear", &Cxgridstrs::_scxGridThisYear },
         { "Cxgridstrs::_scxGridNextYear", &Cxgridstrs::_scxGridNextYear },
         { "Cxgridstrs::_scxGridPast", &Cxgridstrs::_scxGridPast },
         { "Cxgridstrs::_scxGridFuture", &Cxgridstrs::_scxGridFuture },
         { "Cxgridstrs::_scxGridMonthFormat", &Cxgridstrs::_scxGridMonthFormat },
         { "Cxgridstrs::_scxGridYearFormat", &Cxgridstrs::_scxGridYearFormat },
         { "Cxgridstrs::_scxGridChartCategoriesDisplayText", &Cxgridstrs::_scxGridChartCategoriesDisplayText },
         { "Cxgridstrs::_scxGridChartValueHintFormat", &Cxgridstrs::_scxGridChartValueHintFormat },
         { "Cxgridstrs::_scxGridChartPercentValueTickMarkLabelFormat", &Cxgridstrs::_scxGridChartPercentValueTickMarkLabelFormat },
         { "Cxgridstrs::_scxGridChartToolBoxDataLevels", &Cxgridstrs::_scxGridChartToolBoxDataLevels },
         { "Cxgridstrs::_scxGridChartToolBoxDataLevelSelectValue", &Cxgridstrs::_scxGridChartToolBoxDataLevelSelectValue },
         { "Cxgridstrs::_scxGridChartToolBoxCustomizeButtonCaption", &Cxgridstrs::_scxGridChartToolBoxCustomizeButtonCaption },
         { "Cxgridstrs::_scxGridChartNoneDiagramDisplayText", &Cxgridstrs::_scxGridChartNoneDiagramDisplayText },
         { "Cxgridstrs::_scxGridChartColumnDiagramDisplayText", &Cxgridstrs::_scxGridChartColumnDiagramDisplayText },
         { "Cxgridstrs::_scxGridChartBarDiagramDisplayText", &Cxgridstrs::_scxGridChartBarDiagramDisplayText },
         { "Cxgridstrs::_scxGridChartLineDiagramDisplayText", &Cxgridstrs::_scxGridChartLineDiagramDisplayText },
         { "Cxgridstrs::_scxGridChartAreaDiagramDisplayText", &Cxgridstrs::_scxGridChartAreaDiagramDisplayText },
         { "Cxgridstrs::_scxGridChartPieDiagramDisplayText", &Cxgridstrs::_scxGridChartPieDiagramDisplayText },
         { "Cxgridstrs::_scxGridChartStackedBarDiagramDisplayText", &Cxgridstrs::_scxGridChartStackedBarDiagramDisplayText },
         { "Cxgridstrs::_scxGridChartStackedColumnDiagramDisplayText", &Cxgridstrs::_scxGridChartStackedColumnDiagramDisplayText },
         { "Cxgridstrs::_scxGridChartStackedAreaDiagramDisplayText", &Cxgridstrs::_scxGridChartStackedAreaDiagramDisplayText },
         { "Cxgridstrs::_scxGridChartCustomizationFormSeriesPageCaption", &Cxgridstrs::_scxGridChartCustomizationFormSeriesPageCaption },
         { "Cxgridstrs::_scxGridChartCustomizationFormSortBySeries", &Cxgridstrs::_scxGridChartCustomizationFormSortBySeries },
         { "Cxgridstrs::_scxGridChartCustomizationFormNoSortedSeries", &Cxgridstrs::_scxGridChartCustomizationFormNoSortedSeries },
         { "Cxgridstrs::_scxGridChartCustomizationFormDataGroupsPageCaption", &Cxgridstrs::_scxGridChartCustomizationFormDataGroupsPageCaption },
         { "Cxgridstrs::_scxGridChartCustomizationFormOptionsPageCaption", &Cxgridstrs::_scxGridChartCustomizationFormOptionsPageCaption },
         { "Cxgridstrs::_scxGridChartLegend", &Cxgridstrs::_scxGridChartLegend },
         { "Cxgridstrs::_scxGridChartLegendKeyBorder", &Cxgridstrs::_scxGridChartLegendKeyBorder },
         { "Cxgridstrs::_scxGridChartPosition", &Cxgridstrs::_scxGridChartPosition },
         { "Cxgridstrs::_scxGridChartPositionDefault", &Cxgridstrs::_scxGridChartPositionDefault },
         { "Cxgridstrs::_scxGridChartPositionNone", &Cxgridstrs::_scxGridChartPositionNone },
         { "Cxgridstrs::_scxGridChartPositionLeft", &Cxgridstrs::_scxGridChartPositionLeft },
         { "Cxgridstrs::_scxGridChartPositionTop", &Cxgridstrs::_scxGridChartPositionTop },
         { "Cxgridstrs::_scxGridChartPositionRight", &Cxgridstrs::_scxGridChartPositionRight },
         { "Cxgridstrs::_scxGridChartPositionBottom", &Cxgridstrs::_scxGridChartPositionBottom },
         { "Cxgridstrs::_scxGridChartAlignment", &Cxgridstrs::_scxGridChartAlignment },
         { "Cxgridstrs::_scxGridChartAlignmentDefault", &Cxgridstrs::_scxGridChartAlignmentDefault },
         { "Cxgridstrs::_scxGridChartAlignmentStart", &Cxgridstrs::_scxGridChartAlignmentStart },
         { "Cxgridstrs::_scxGridChartAlignmentCenter", &Cxgridstrs::_scxGridChartAlignmentCenter },
         { "Cxgridstrs::_scxGridChartAlignmentEnd", &Cxgridstrs::_scxGridChartAlignmentEnd },
         { "Cxgridstrs::_scxGridChartOrientation", &Cxgridstrs::_scxGridChartOrientation },
         { "Cxgridstrs::_scxGridChartOrientationDefault", &Cxgridstrs::_scxGridChartOrientationDefault },
         { "Cxgridstrs::_scxGridChartOrientationHorizontal", &Cxgridstrs::_scxGridChartOrientationHorizontal },
         { "Cxgridstrs::_scxGridChartOrientationVertical", &Cxgridstrs::_scxGridChartOrientationVertical },
         { "Cxgridstrs::_scxGridChartBorder", &Cxgridstrs::_scxGridChartBorder },
         { "Cxgridstrs::_scxGridChartTitle", &Cxgridstrs::_scxGridChartTitle },
         { "Cxgridstrs::_scxGridChartToolBox", &Cxgridstrs::_scxGridChartToolBox },
         { "Cxgridstrs::_scxGridChartDiagramSelector", &Cxgridstrs::_scxGridChartDiagramSelector },
         { "Cxgridstrs::_scxGridChartOther", &Cxgridstrs::_scxGridChartOther },
         { "Cxgridstrs::_scxGridChartValueHints", &Cxgridstrs::_scxGridChartValueHints },
         { "Cxgridstrs::_scxGridLayoutViewCustomizeFormOk", &Cxgridstrs::_scxGridLayoutViewCustomizeFormOk },
         { "Cxgridstrs::_scxGridLayoutViewCustomizeFormCancel", &Cxgridstrs::_scxGridLayoutViewCustomizeFormCancel },
         { "Cxgridstrs::_scxGridLayoutViewCustomizeFormApply", &Cxgridstrs::_scxGridLayoutViewCustomizeFormApply },
         { "Cxgridstrs::_scxGridLayoutViewCustomizeWarningDialogCaption", &Cxgridstrs::_scxGridLayoutViewCustomizeWarningDialogCaption },
         { "Cxgridstrs::_scxGridLayoutViewCustomizeWarningDialogMessage", &Cxgridstrs::_scxGridLayoutViewCustomizeWarningDialogMessage },
         { "Cxgridstrs::_scxGridLayoutViewCustomizeLayoutButtonCaption", &Cxgridstrs::_scxGridLayoutViewCustomizeLayoutButtonCaption },
         { "Cxgridstrs::_scxGridLayoutViewCustomizeFormTemplateCard", &Cxgridstrs::_scxGridLayoutViewCustomizeFormTemplateCard },
         { "Cxgridstrs::_scxGridLayoutViewCustomizeFormViewLayout", &Cxgridstrs::_scxGridLayoutViewCustomizeFormViewLayout },
         { "Cxgridstrs::_scxGridLayoutViewRecordCaptionDefaultMask", &Cxgridstrs::_scxGridLayoutViewRecordCaptionDefaultMask },
         { "Cxgridstrs::_scxGridLockedStateImageText", &Cxgridstrs::_scxGridLockedStateImageText },
         { "Cxgridstrs::_scxGridInplaceEditFormButtonCancel", &Cxgridstrs::_scxGridInplaceEditFormButtonCancel },
         { "Cxgridstrs::_scxGridInplaceEditFormButtonClose", &Cxgridstrs::_scxGridInplaceEditFormButtonClose },
         { "Cxgridstrs::_scxGridInplaceEditFormButtonUpdate", &Cxgridstrs::_scxGridInplaceEditFormButtonUpdate },
         { "Cxgridstrs::_scxGridInplaceEditFormSaveChangesQuery", &Cxgridstrs::_scxGridInplaceEditFormSaveChangesQuery },
         { "Cxgridstrs::_scxGridDataRowFixingPopupCommandUnfix", &Cxgridstrs::_scxGridDataRowFixingPopupCommandUnfix },
         { "Cxgridstrs::_scxGridDataRowFixingPopupCommandFixToTop", &Cxgridstrs::_scxGridDataRowFixingPopupCommandFixToTop },
         { "Cxgridstrs::_scxGridDataRowFixingPopupCommandFixToBottom", &Cxgridstrs::_scxGridDataRowFixingPopupCommandFixToBottom },
         { "Cxgridstrs::_scxGridColumnHeaderUndefinedFilterButtonUIAName", &Cxgridstrs::_scxGridColumnHeaderUndefinedFilterButtonUIAName },
         { "Cxgridstrs::_scxGridColumnHeaderDefinedFilterButtonUIAName", &Cxgridstrs::_scxGridColumnHeaderDefinedFilterButtonUIAName },
         { "Cxgridstrs::_scxGridColumnHeaderFilteredUIAStatus", &Cxgridstrs::_scxGridColumnHeaderFilteredUIAStatus }
      };
   }


std::vector<TResourceEntry> CreateGridPopupMenuResourceList() {
   return {
      { "Cxgridpopupmenuconsts::_cxSGridNone", &Cxgridpopupmenuconsts::_cxSGridNone },
      { "Cxgridpopupmenuconsts::_cxSGridSortColumnAsc", &Cxgridpopupmenuconsts::_cxSGridSortColumnAsc },
      { "Cxgridpopupmenuconsts::_cxSGridSortColumnDesc", &Cxgridpopupmenuconsts::_cxSGridSortColumnDesc },
      { "Cxgridpopupmenuconsts::_cxSGridClearSorting", &Cxgridpopupmenuconsts::_cxSGridClearSorting },
      { "Cxgridpopupmenuconsts::_cxSGridGroupByThisField", &Cxgridpopupmenuconsts::_cxSGridGroupByThisField },
      { "Cxgridpopupmenuconsts::_cxSGridRemoveThisGroupItem", &Cxgridpopupmenuconsts::_cxSGridRemoveThisGroupItem },
      { "Cxgridpopupmenuconsts::_cxSGridGroupByBox", &Cxgridpopupmenuconsts::_cxSGridGroupByBox },
      { "Cxgridpopupmenuconsts::_cxSGridAlignmentSubMenu", &Cxgridpopupmenuconsts::_cxSGridAlignmentSubMenu },
      { "Cxgridpopupmenuconsts::_cxSGridAlignLeft", &Cxgridpopupmenuconsts::_cxSGridAlignLeft },
      { "Cxgridpopupmenuconsts::_cxSGridAlignRight", &Cxgridpopupmenuconsts::_cxSGridAlignRight },
      { "Cxgridpopupmenuconsts::_cxSGridAlignCenter", &Cxgridpopupmenuconsts::_cxSGridAlignCenter },
      { "Cxgridpopupmenuconsts::_cxSGridRemoveColumn", &Cxgridpopupmenuconsts::_cxSGridRemoveColumn },
      { "Cxgridpopupmenuconsts::_cxSGridFieldChooser", &Cxgridpopupmenuconsts::_cxSGridFieldChooser },
      { "Cxgridpopupmenuconsts::_cxSGridBestFit", &Cxgridpopupmenuconsts::_cxSGridBestFit },
      { "Cxgridpopupmenuconsts::_cxSGridBestFitAllColumns", &Cxgridpopupmenuconsts::_cxSGridBestFitAllColumns },
      { "Cxgridpopupmenuconsts::_cxSGridShowFooter", &Cxgridpopupmenuconsts::_cxSGridShowFooter },
      { "Cxgridpopupmenuconsts::_cxSGridShowGroupFooter", &Cxgridpopupmenuconsts::_cxSGridShowGroupFooter },
      { "Cxgridpopupmenuconsts::_cxSGridShowFindPanel", &Cxgridpopupmenuconsts::_cxSGridShowFindPanel },
     // { "Cxgridpopupmenuconsts::_cxSGridShowExpressionEditor", &Cxgridpopupmenuconsts::_cxSGridShowExpressionEditor },
      { "Cxgridpopupmenuconsts::_cxSGridFullExpand", &Cxgridpopupmenuconsts::_cxSGridFullExpand },
      { "Cxgridpopupmenuconsts::_cxSGridFullCollapse", &Cxgridpopupmenuconsts::_cxSGridFullCollapse },
      { "Cxgridpopupmenuconsts::_cxSGridClearGrouping", &Cxgridpopupmenuconsts::_cxSGridClearGrouping },
      { "Cxgridpopupmenuconsts::_cxSGridHideGroupByBox", &Cxgridpopupmenuconsts::_cxSGridHideGroupByBox },
      { "Cxgridpopupmenuconsts::_cxSGridNoneMenuItem", &Cxgridpopupmenuconsts::_cxSGridNoneMenuItem },
      { "Cxgridpopupmenuconsts::_cxSGridSumMenuItem", &Cxgridpopupmenuconsts::_cxSGridSumMenuItem },
      { "Cxgridpopupmenuconsts::_cxSGridMinMenuItem", &Cxgridpopupmenuconsts::_cxSGridMinMenuItem },
      { "Cxgridpopupmenuconsts::_cxSGridMaxMenuItem", &Cxgridpopupmenuconsts::_cxSGridMaxMenuItem },
      { "Cxgridpopupmenuconsts::_cxSGridCountMenuItem", &Cxgridpopupmenuconsts::_cxSGridCountMenuItem },
      { "Cxgridpopupmenuconsts::_cxSGridAvgMenuItem", &Cxgridpopupmenuconsts::_cxSGridAvgMenuItem },
      { "Cxgridpopupmenuconsts::_cxSGridSortByGroupValues", &Cxgridpopupmenuconsts::_cxSGridSortByGroupValues },
      { "Cxgridpopupmenuconsts::_cxSGridSortBySummaryCaption", &Cxgridpopupmenuconsts::_cxSGridSortBySummaryCaption },
      { "Cxgridpopupmenuconsts::_cxSGridSortBySummary", &Cxgridpopupmenuconsts::_cxSGridSortBySummary }
      };
   }

   
void WriteResources(std::ofstream& out) {
   auto resText = Dxcore::cxGetResourceString(&Cxgridstrs::_scxGridRecursiveLevels);

   ///*
   for(auto [text, resID] : CreateGridResourceList()) {
      auto resText = EscapeCppStringLiteral(ConvertTo<std::string>(cxGetResourceString(resID)));
      std::println(out, "{};{}", text, ConvertTo<std::string>(resText));
      }

   for(auto [text, resID] : CreateGridPopupMenuResourceList()) {
      auto resText = EscapeCppStringLiteral(ConvertTo<std::string>(cxGetResourceString(resID)));
      std::println(out, "{};{}", text, resText);
      }

   //*/
   }
      
} // end of namespace
