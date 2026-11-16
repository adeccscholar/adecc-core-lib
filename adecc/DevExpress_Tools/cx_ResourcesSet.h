#pragma once

#include <cxClasses.hpp>
#include <dxCore.hpp>

#include <cxGridStrs.hpp>
#include <cxGridPopupMenuConsts.hpp>

namespace adecc::delphi::vcl::devexpress {

void SetGridResources() {
   cxSetResourceString(&Cxgridstrs::_scxGridNoDataInfoText, L"Keine Daten vorhanden");
   cxSetResourceString(&Cxgridstrs::_scxGridFilterRowInfoText, L"Zum Filtern Wert eingeben");
   cxSetResourceString(&Cxgridstrs::_scxGridNewItemRowInfoText, L"Zum Hinzufügen hier klicken");

   cxSetResourceString(&Cxgridstrs::_scxGridFindPanelClearButtonCaption, L"Löschen");
   cxSetResourceString(&Cxgridstrs::_scxGridFindPanelFindButtonCaption, L"Suchen");
   cxSetResourceString(&Cxgridstrs::_scxGridFindPanelNextButtonCaption, L"Weiter");
   cxSetResourceString(&Cxgridstrs::_scxGridFindPanelPreviousButtonCaption, L"Zurück");
   cxSetResourceString(&Cxgridstrs::_scxGridFindPanelInfoText, L"Suchtext eingeben");

   cxSetResourceString(&Cxgridstrs::_scxGridFilterIsEmpty, L"Ist leer");

   cxSetResourceString(&Cxgridstrs::_scxGridCustomizationFormCaption, L"Anpassen");
   cxSetResourceString(&Cxgridstrs::_scxGridCustomizationFormColumnsPageCaption, L"Spalten");
   cxSetResourceString(&Cxgridstrs::_scxGridCustomizationFormBandsPageCaption, L"Bänder");
   cxSetResourceString(&Cxgridstrs::_scxGridCustomizationFormRowsPageCaption, L"Zeilen");

   cxSetResourceString(&Cxgridstrs::_scxGridGroupByBoxCaption, L"Ziehen Sie eine Spalte hierher, um danach zu gruppieren");

   cxSetResourceString(&Cxgridstrs::_scxGridFilterApplyButtonCaption, L"Anwenden");
   cxSetResourceString(&Cxgridstrs::_scxGridFilterCustomizeButtonCaption, L"Anpassen");

   cxSetResourceString(&Cxgridstrs::_scxGridColumnsQuickCustomizationHint, L"Spalten zum Ein oder Ausblenden ziehen");
   cxSetResourceString(&Cxgridstrs::_scxGridBandsQuickCustomizationHint, L"Bänder zum Ein oder Ausblenden ziehen");

   cxSetResourceString(&Cxgridstrs::_scxGridDeletingFocusedConfirmationText, L"Soll der aktuelle Datensatz gelöscht werden?");
   cxSetResourceString(&Cxgridstrs::_scxGridDeletingSelectedConfirmationText, L"Sollen die ausgewählten Datensätze gelöscht werden?");

   cxSetResourceString(&Cxgridstrs::_scxGridYesterday, L"Gestern");
   cxSetResourceString(&Cxgridstrs::_scxGridToday, L"Heute");
   cxSetResourceString(&Cxgridstrs::_scxGridTomorrow, L"Morgen");

   cxSetResourceString(&Cxgridstrs::_scxGridLast7Days, L"Letzte 7 Tage");
   cxSetResourceString(&Cxgridstrs::_scxGridLast14Days, L"Letzte 14 Tage");
   cxSetResourceString(&Cxgridstrs::_scxGridLast30Days, L"Letzte 30 Tage");

   cxSetResourceString(&Cxgridstrs::_scxGridNext7Days, L"Nächste 7 Tage");
   cxSetResourceString(&Cxgridstrs::_scxGridNext14Days, L"Nächste 14 Tage");
   cxSetResourceString(&Cxgridstrs::_scxGridNext30Days, L"Nächste 30 Tage");

   cxSetResourceString(&Cxgridstrs::_scxGridLastWeek, L"Letzte Woche");
   cxSetResourceString(&Cxgridstrs::_scxGridThisWeek, L"Diese Woche");
   cxSetResourceString(&Cxgridstrs::_scxGridNextWeek, L"Nächste Woche");

   cxSetResourceString(&Cxgridstrs::_scxGridLastMonth, L"Letzter Monat");
   cxSetResourceString(&Cxgridstrs::_scxGridThisMonth, L"Dieser Monat");
   cxSetResourceString(&Cxgridstrs::_scxGridNextMonth, L"Nächster Monat");

   cxSetResourceString(&Cxgridstrs::_scxGridLastYear, L"Letztes Jahr");
   cxSetResourceString(&Cxgridstrs::_scxGridThisYear, L"Dieses Jahr");
   cxSetResourceString(&Cxgridstrs::_scxGridNextYear, L"Nächstes Jahr");

   cxSetResourceString(&Cxgridstrs::_scxGridPast, L"Vergangenheit");
   cxSetResourceString(&Cxgridstrs::_scxGridFuture, L"Zukunft");

   cxSetResourceString(&Cxgridstrs::_scxGridInplaceEditFormButtonCancel, L"Abbrechen");
   cxSetResourceString(&Cxgridstrs::_scxGridInplaceEditFormButtonClose, L"Schließen");
   cxSetResourceString(&Cxgridstrs::_scxGridInplaceEditFormButtonUpdate, L"Übernehmen");
   cxSetResourceString(&Cxgridstrs::_scxGridInplaceEditFormSaveChangesQuery, L"Sollen die Änderungen gespeichert werden?");
  
   cxSetResourceString(&Cxgridpopupmenuconsts::_cxSGridBestFit, L"Optimale Breite");
   cxSetResourceString(&Cxgridpopupmenuconsts::_cxSGridBestFitAllColumns, L"Alle Spalten optimal anpassen");
   cxSetResourceString(&Cxgridpopupmenuconsts::_cxSGridClearSorting, L"Sortierung entfernen");
   cxSetResourceString(&Cxgridpopupmenuconsts::_cxSGridFieldChooser, L"Spaltenauswahl");
   cxSetResourceString(&Cxgridpopupmenuconsts::_cxSGridShowFindPanel, L"Suchleiste");
   cxSetResourceString(&Cxgridpopupmenuconsts::_cxSGridGroupByBox, L"Gruppierungsleiste");
   cxSetResourceString(&Cxgridpopupmenuconsts::_cxSGridGroupByThisField, L"Nach dieser Spalte gruppieren");
   cxSetResourceString(&Cxgridpopupmenuconsts::_cxSGridRemoveColumn, L"Diese Spalte ausblenden");
   cxSetResourceString(&Cxgridpopupmenuconsts::_cxSGridSortColumnAsc, L"Aufsteigend sortieren");
   cxSetResourceString(&Cxgridpopupmenuconsts::_cxSGridSortColumnDesc, L"Absteigend sortieren");
   cxSetResourceString(&Cxgridpopupmenuconsts::_cxSGridHideGroupByBox, L"Gruppierungsleiste ausblenden");
   cxSetResourceString(&Cxgridpopupmenuconsts::_cxSGridFullExpand, L"Alles ausklappen");
   cxSetResourceString(&Cxgridpopupmenuconsts::_cxSGridFullCollapse, L"Alles einklappen");
   cxSetResourceString(&Cxgridpopupmenuconsts::_cxSGridSortByGroupValues, L"Nach Gruppenwert sortieren");
   cxSetResourceString(&Cxgridpopupmenuconsts::_cxSGridSortBySummaryCaption, L"Nach Gruppensumme sortieren:");
   }

} // end of namespace
