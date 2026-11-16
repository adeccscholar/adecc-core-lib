#pragma once

#include "convert_core.h"
#include "cxgrid_wrapper.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
// #include <boost/property_tree/json_parser.hpp>

#include <string>
#include <vector>
#include <tuple>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <filesystem>

namespace cx = adecc::delphi::vcl::devexpress;

enum class EXmlEncoding : uint32_t {
   utf8,
   windows_1252,
   unknown
};

struct SGridLayoutXml {
   cx::vecBandCaptions<adecc::AnsiStreamPolicy> vecBands;
   cx::vecBandedCaptions<adecc::AnsiStreamPolicy> vecColumns;
};


namespace cxcfg {

namespace pt = boost::property_tree;

/*! \brief String nach bool lesen. */
inline bool ToBool(std::string const& strValue) {
   if(strValue == "true" || strValue == "1") {
      return true;
      }
   if(strValue == "false" || strValue == "0") {
      return false;
      }
   throw std::invalid_argument{"invalid bool value: " + strValue};
   }

/*! \brief Alignment aus Text lesen. */
inline adecc::EAlignmentType ToAlignment(std::string const& strValue) {
   if(strValue == "left") {
      return adecc::EAlignmentType::left;
      }
   if(strValue == "right") {
      return adecc::EAlignmentType::right;
      }
   if(strValue == "center") {
      return adecc::EAlignmentType::center;
      }
   if(strValue == "unknown") {
      return adecc::EAlignmentType::unknown;
      }
   throw std::invalid_argument{"invalid alignment: " + strValue};
   }

/*! \brief Grid Typ aus Text lesen. */
inline adecc::delphi::vcl::devexpress::EGridColType ToGridColType(std::string const& strValue) {
   using enum_ty = adecc::delphi::vcl::devexpress::EGridColType;

   if(strValue == "text") {
      return enum_ty::text;
      }
   if(strValue == "memo") {
      return enum_ty::memo;
      }
   if(strValue == "integer") {
      return enum_ty::integer;
      }
   if(strValue == "floating") {
      return enum_ty::floating;
      }
   if(strValue == "currency") {
      return enum_ty::currency;
      }
   if(strValue == "percent") {
      return enum_ty::percent;
      }
   if(strValue == "count") {
      return enum_ty::count;
      }
   if(strValue == "checkbox") {
      return enum_ty::checkbox;
      }
   if(strValue == "boolean") {
      return enum_ty::boolean;
      }
   if(strValue == "date") {
      return enum_ty::date;
      }
   if(strValue == "datetime") {
      return enum_ty::datetime;
      }
   if(strValue == "time") {
      return enum_ty::time;
      }
   if(strValue == "combobox") {
      return enum_ty::combobox;
      }

   throw std::invalid_argument{"invalid grid column type: " + strValue};
   }

/*! \brief Aggregat aus Text lesen. */
inline adecc::delphi::vcl::devexpress::EGridColAggregate ToGridColAggregate(std::string const& strValue) {
   using enum_ty = adecc::delphi::vcl::devexpress::EGridColAggregate;

   if(strValue == "none") {
      return enum_ty::none;
      }
   if(strValue == "count") {
      return enum_ty::count;
      }
   if(strValue == "distinct_count") {
      return enum_ty::distinct_count;
      }
   if(strValue == "sum") {
      return enum_ty::sum;
      }
   if(strValue == "average") {
      return enum_ty::average;
      }
   if(strValue == "min") {
      return enum_ty::min;
      }
   if(strValue == "max") {
      return enum_ty::max;
      }

   throw std::invalid_argument{"invalid aggregate: " + strValue};
   }

/*! \brief XML Attribut als std::string lesen. */
inline std::string ReadAttrString(pt::ptree const& theNode, std::string const& strAttrName) {
   return theNode.get<std::string>("<xmlattr>." + strAttrName);
   }

/*! \brief XML Attribut als std::string mit Default lesen. */
inline std::string ReadAttrString(pt::ptree const& theNode,
                                  std::string const& strAttrName,
                                  std::string const& strDefault) {
   return theNode.get<std::string>("<xmlattr>." + strAttrName, strDefault);
   }

/*! \brief XML Attribut als int lesen. */
inline int ReadAttrInt(pt::ptree const& theNode, std::string const& strAttrName) {
   return theNode.get<int>("<xmlattr>." + strAttrName);
   }

/*! \brief XML Attribut als int mit Default lesen. */
inline int ReadAttrInt(pt::ptree const& theNode,
                       std::string const& strAttrName,
                       int const iDefault) {
   return theNode.get<int>("<xmlattr>." + strAttrName, iDefault);
   }

/*! \brief XML Attribut als bool lesen. */
inline bool ReadAttrBool(pt::ptree const& theNode, std::string const& strAttrName) {
   return ToBool(ReadAttrString(theNode, strAttrName));
   }

/*! \brief XML Attribut als bool mit Default lesen. */
inline bool ReadAttrBool(pt::ptree const& theNode,
                         std::string const& strAttrName,
                         bool const bDefault) {
   return ToBool(ReadAttrString(theNode, strAttrName, bDefault ? "true" : "false"));
   }

/*! \brief XML Attribut als Alignment lesen. */
inline adecc::EAlignmentType ReadAttrAlignment(pt::ptree const& theNode, std::string const& strAttrName) {
   return ToAlignment(ReadAttrString(theNode, strAttrName));
   }

/*! \brief XML Attribut als Grid Typ lesen. */
inline adecc::delphi::vcl::devexpress::EGridColType ReadAttrGridColType(pt::ptree const& theNode,
                                                                        std::string const& strAttrName) {
   return ToGridColType(ReadAttrString(theNode, strAttrName));
   }

/*! \brief XML Attribut als Aggregat lesen. */
inline adecc::delphi::vcl::devexpress::EGridColAggregate ReadAttrGridColAggregate(pt::ptree const& theNode,
                                                                                  std::string const& strAttrName) {
   return ToGridColAggregate(ReadAttrString(theNode, strAttrName));
   }

} // namespace cxcfg




inline cx::vecBandCaptions<adecc::AnsiStreamPolicy>
LoadBandCaptionsFromPtree(boost::property_tree::ptree const& theBandsNode) {
   using result_ty = cx::vecBandCaptions<adecc::AnsiStreamPolicy>;

   result_ty vecResult;

   for(auto const& [strNodeName, theNode] : theBandsNode) {
      if(strNodeName != "Band") {
         continue;
         }

      auto const& a = theNode.get_child("<xmlattr>");

      std::string const strCaption = a.get<std::string>("caption", "");
      bool const bVisible = cxcfg::ToBool(a.get<std::string>("visible", "true"));

      vecResult.emplace_back(
         cx::tplBandCaption<adecc::AnsiStreamPolicy>{
            strCaption,
            bVisible
         }
      );
      }

   return vecResult;
}

inline void SaveBandCaptionsToPtree(
      boost::property_tree::ptree& theBandsNode,
      cx::vecBandCaptions<adecc::AnsiStreamPolicy> const& vecBands) {
   for(auto const& t : vecBands) {
      boost::property_tree::ptree theBandNode;
      theBandNode.put("<xmlattr>.caption", std::get<0>(t));
      theBandNode.put("<xmlattr>.visible", std::get<1>(t) ? "true" : "false");
      theBandsNode.add_child("Band", theBandNode);
      }
}






inline cx::vecBandedCaptions<adecc::AnsiStreamPolicy>
LoadBandedCaptionsFromPtree(boost::property_tree::ptree const& theColumnsNode) {
   using result_ty = cx::vecBandedCaptions<adecc::AnsiStreamPolicy>;
   using tuple_ty  = cx::tplBandedCaption<adecc::AnsiStreamPolicy>;

   result_ty vecResult;

   for(auto const& [strNodeName, theNode] : theColumnsNode) {
      if(strNodeName != "Column") {
         continue;
         }

      std::string const strCaption   = cxcfg::ReadAttrString(theNode, "caption");
      int const iWidth               = cxcfg::ReadAttrInt(theNode, "width");
      auto const eAlignment          = cxcfg::ReadAttrAlignment(theNode, "alignment");
      int const iColIndex            = cxcfg::ReadAttrInt(theNode, "colIndex", 0);
      int const iBandIndex           = cxcfg::ReadAttrInt(theNode, "bandIndex");
      bool const bVisible            = cxcfg::ReadAttrBool(theNode, "visible");
      bool const bEditable           = cxcfg::ReadAttrBool(theNode, "editable");
      bool const bSortable           = cxcfg::ReadAttrBool(theNode, "sortable");
      bool const bFilterable         = cxcfg::ReadAttrBool(theNode, "filterable");
      bool const bGroupable          = cxcfg::ReadAttrBool(theNode, "groupable");
      bool const bSearchable         = cxcfg::ReadAttrBool(theNode, "searchable");
      auto const eType               = cxcfg::ReadAttrGridColType(theNode, "type");
      int const iScale               = cxcfg::ReadAttrInt(theNode, "scale");
      auto const eAggregate          = cxcfg::ReadAttrGridColAggregate(theNode, "aggregate");
      std::string const strTag       = cxcfg::ReadAttrString(theNode, "tag", "");

      vecResult.emplace_back(tuple_ty{
         strCaption,
         iWidth,
         eAlignment,
         iColIndex,
         iBandIndex,
         bVisible,
         bEditable,
         bSortable,
         bFilterable,
         bGroupable,
         bSearchable,
         eType,
         iScale,
         eAggregate,
         strTag
      });
      }

   return vecResult;
}


inline void SaveBandedCaptionsToPtree(
      boost::property_tree::ptree& theColumnsNode,
      cx::vecBandedCaptions<adecc::AnsiStreamPolicy> const& vecCaps) {

   auto ToStringAlignment = [](adecc::EAlignmentType const eValue) -> std::string {
      switch(eValue) {
         case adecc::EAlignmentType::left:    return "left";
         case adecc::EAlignmentType::right:   return "right";
         case adecc::EAlignmentType::center:  return "center";
         case adecc::EAlignmentType::unknown: [[fallthrough]];
         default:                             return "unknown";
         }
      };

   auto ToStringType = [](cx::EGridColType const eValue) -> std::string {
      switch(eValue) {
         case cx::EGridColType::text:      return "text";
         case cx::EGridColType::memo:      return "memo";
         case cx::EGridColType::integer:   return "integer";
         case cx::EGridColType::floating:  return "floating";
         case cx::EGridColType::currency:  return "currency";
         case cx::EGridColType::percent:   return "percent";
         case cx::EGridColType::count:     return "count";
         case cx::EGridColType::checkbox:  return "checkbox";
         case cx::EGridColType::boolean:   return "boolean";
         case cx::EGridColType::date:      return "date";
         case cx::EGridColType::datetime:  return "datetime";
         case cx::EGridColType::time:      return "time";
         case cx::EGridColType::combobox:  return "combobox";
         }
      return "text";
      };

   auto ToStringAggregate = [](cx::EGridColAggregate const eValue) -> std::string {
      switch(eValue) {
         case cx::EGridColAggregate::none:           return "none";
         case cx::EGridColAggregate::count:          return "count";
         case cx::EGridColAggregate::distinct_count: return "distinct_count";
         case cx::EGridColAggregate::sum:            return "sum";
         case cx::EGridColAggregate::average:        return "average";
         case cx::EGridColAggregate::min:            return "min";
         case cx::EGridColAggregate::max:            return "max";
         }
      return "none";
      };

   for(auto const& t : vecCaps) {
      boost::property_tree::ptree theNode;
      theNode.put("<xmlattr>.caption",    std::get<0>(t));
      theNode.put("<xmlattr>.width",      std::get<1>(t));
      theNode.put("<xmlattr>.alignment",  ToStringAlignment(std::get<2>(t)));
      theNode.put("<xmlattr>.colIndex",   std::get<3>(t));
      theNode.put("<xmlattr>.bandIndex",  std::get<4>(t));
      theNode.put("<xmlattr>.visible",    std::get<5>(t) ? "true" : "false");
      theNode.put("<xmlattr>.editable",   std::get<6>(t) ? "true" : "false");
      theNode.put("<xmlattr>.sortable",   std::get<7>(t) ? "true" : "false");
      theNode.put("<xmlattr>.filterable", std::get<8>(t) ? "true" : "false");
      theNode.put("<xmlattr>.groupable",  std::get<9>(t) ? "true" : "false");
      theNode.put("<xmlattr>.searchable", std::get<10>(t) ? "true" : "false");
      theNode.put("<xmlattr>.type",       ToStringType(std::get<11>(t)));
      theNode.put("<xmlattr>.scale",      std::get<12>(t));
      theNode.put("<xmlattr>.aggregate",  ToStringAggregate(std::get<13>(t)));
      theNode.put("<xmlattr>.tag",        std::get<14>(t));

      theColumnsNode.add_child("Column", theNode);
      }
}

inline SGridLayoutXml LoadGridLayoutFromXml(
      std::filesystem::path const& fileName,
      std::string const& strFormName) {

   namespace pt = boost::property_tree;

   pt::ptree theTree;
   pt::read_xml(fileName.string(), theTree, pt::xml_parser::trim_whitespace);

   auto const& theRoot = theTree.get_child("GridLayouts");

   for(auto const& [strNodeName, theFormNode] : theRoot) {
      if(strNodeName != "Grid") {
         continue;
         }

      auto const& a = theFormNode.get_child("<xmlattr>");
      if(a.get<std::string>("name", "") != strFormName) {
         continue;
         }

      SGridLayoutXml theResult;
      theResult.vecBands = LoadBandCaptionsFromPtree(theFormNode.get_child("Bands"));
      theResult.vecColumns = LoadBandedCaptionsFromPtree(theFormNode.get_child("Columns"));
      return theResult;
      }

   throw std::runtime_error{"LoadGridLayoutFromXml: grid not found: " + strFormName};
}

inline void SaveGridLayoutToXml(
      std::filesystem::path const& fileName,
      std::string const& strFormName,
      cx::vecBandCaptions<adecc::AnsiStreamPolicy> const& vecBands,
      cx::vecBandedCaptions<adecc::AnsiStreamPolicy> const& vecCaps) {

   namespace pt = boost::property_tree;

   pt::ptree theTree;
   pt::ptree theRoot;
   pt::ptree theFormNode;
   pt::ptree theBandsNode;
   pt::ptree theColumnsNode;

   theRoot.put("<xmlattr>.version", 1);
   theFormNode.put("<xmlattr>.name", strFormName);

   SaveBandCaptionsToPtree(theBandsNode, vecBands);
   SaveBandedCaptionsToPtree(theColumnsNode, vecCaps);

   theFormNode.add_child("Bands", theBandsNode);
   theFormNode.add_child("Columns", theColumnsNode);
   theRoot.add_child("Grid", theFormNode);
   theTree.add_child("GridLayouts", theRoot);

   pt::xml_writer_settings<std::string> const theSettings(' ', 3);
   pt::write_xml(fileName.string(), theTree, std::locale{}, theSettings);
}


inline void UpsertGridLayoutToXml(
      std::filesystem::path const& fileName,
      std::string const& strFormName,
      cx::vecBandCaptions<adecc::AnsiStreamPolicy> const& vecBands,
      cx::vecBandedCaptions<adecc::AnsiStreamPolicy> const& vecCaps) {

   namespace pt = boost::property_tree;

   pt::ptree theTree;

   try {
      pt::read_xml(fileName.string(), theTree, pt::xml_parser::trim_whitespace);
      }
   catch(...) {
      theTree.clear();
      theTree.put("GridLayouts.<xmlattr>.version", 1);
      }

   if(!theTree.get_child_optional("GridLayouts")) {
      theTree.put("GridLayouts.<xmlattr>.version", 1);
      }

   auto& theRoot = theTree.get_child("GridLayouts");

   pt::ptree theNewRoot;
   if(auto const v = theRoot.get_optional<std::string>("<xmlattr>.version"); v) {
      theNewRoot.put("<xmlattr>.version", *v);
      }
   else {
      theNewRoot.put("<xmlattr>.version", 1);
      }

   for(auto const& [strNodeName, theNode] : theRoot) {
      if(strNodeName != "Grid") {
         theNewRoot.add_child(strNodeName, theNode);
         continue;
         }

      auto const strExistingName = theNode.get<std::string>("<xmlattr>.name", "");
      if(strExistingName != strFormName) {
         theNewRoot.add_child("Grid", theNode);
         }
      }

   pt::ptree theFormNode;
   pt::ptree theBandsNode;
   pt::ptree theColumnsNode;

   theFormNode.put("<xmlattr>.name", strFormName);

   SaveBandCaptionsToPtree(theBandsNode, vecBands);
   SaveBandedCaptionsToPtree(theColumnsNode, vecCaps);

   theFormNode.add_child("Bands", theBandsNode);
   theFormNode.add_child("Columns", theColumnsNode);

   theNewRoot.add_child("Grid", theFormNode);

   theTree.put_child("GridLayouts", theNewRoot);

   pt::xml_writer_settings<std::string> const theSettings(' ', 3);
   pt::write_xml(fileName.string(), theTree, std::locale{}, theSettings);
}


/*
UpsertGridLayoutToXml(
   "grid_layouts.xml",
   "MainBookings",
   booking_bands,
   bookings_banded_captions
);


auto const theLayout = LoadGridLayoutFromXml("grid_layouts.xml", "MainBookings");

booking_bands = theLayout.vecBands;
bookings_banded_captions = theLayout.vecColumns;

*/
