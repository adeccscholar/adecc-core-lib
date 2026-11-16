
#pragma once

#include "../cpp_core/convert_core.h"
#include "../cpp_core/convert_fixed.h"

#include <System.hpp>             // AnsiString, UnicodeString, TDateTime, Variant
#include <System.SysUtils.hpp>
#include <System.Variants.hpp>    // VarAsType, VarToStr, varXxx
#include <System.Classes.hpp>


namespace adecc {


// ======================================================
// Strings: std::string / std::wstring / UnicodeString / AnsiString
// ======================================================

// std::string / UnicodeString (via AnsiString 1:1)
template <> struct Convert<std::string, System::UnicodeString> {
   static System::UnicodeString apply(std::string const & s) {
	  return System::UnicodeString(s.c_str());
	  }
   };

template <> struct Convert<System::UnicodeString, std::string> {
   static std::string apply(System::UnicodeString const& u) {
	  System::AnsiString const a = System::AnsiString(u);
	  return std::string(a.c_str());
	  }
   };

// std::string / AnsiString
template <> struct Convert<std::string, System::AnsiString> {
   static System::AnsiString apply(std::string const& s) { return System::AnsiString(s.c_str()); }
   };

template <> struct Convert<System::AnsiString, std::string> {
   static std::string apply(System::AnsiString const& a) { return std::string(a.c_str()); }
   };

// std::wstring / UnicodeString
template <> struct Convert<std::wstring, System::UnicodeString> {
   static System::UnicodeString apply(std::wstring const& ws) { return System::UnicodeString(ws.c_str()); }
   };

template <> struct Convert<System::UnicodeString, std::wstring> {
   static std::wstring apply(System::UnicodeString const& u) { return std::wstring(u.c_str()); }
   };


// UnicodeString / AnsiString (falls direkt gebraucht)
template <> struct Convert<System::AnsiString, System::UnicodeString> {
   static System::UnicodeString apply(System::AnsiString const & a) { return System::UnicodeString(a); }
   };

template <> struct Convert<System::UnicodeString, System::AnsiString> {
   static System::AnsiString apply(System::UnicodeString const & u) { return System::AnsiString(u); }
   };

// ======================================================
// Delphi-Strings) wstring / string wegen Delphi Umweg
// ======================================================

// std::string / std::wstring (über UnicodeString, Hilfsmethode, andere Lösung suchen)
template <> struct Convert<std::string, std::wstring> {
   static std::wstring apply(std::string const& s) {
	  return Convert<System::UnicodeString, std::wstring>::apply(
		 Convert<std::string, System::UnicodeString>::apply(s));
      }
   };

template <> struct Convert<std::wstring, std::string> {
   static std::string apply(std::wstring const & ws) {
	  return Convert<System::UnicodeString, std::string>::apply(
		 Convert<std::wstring, System::UnicodeString>::apply(ws));
      }
   };

// Convert<const char*, UnicodeString>
template <>
struct Convert<char const*, System::UnicodeString> {
   static System::UnicodeString apply(char const* sz) {
      // 1:1 nationale Interpretation via AnsiString, dann nach UnicodeString
      return sz ? System::UnicodeString(sz)  : UnicodeString("");
      }
   };

// Optional: Forward für non-const char*
template <>
struct Convert<char*, System::UnicodeString> {
   static System::UnicodeString apply(char* sz) {
      return Convert<char const*, System::UnicodeString>::apply(sz);
      }
   };

// Optional: explizit leer bei nullptr (z. B. direkte Aufrufe mit nullptr)
template <>
struct Convert<std::nullptr_t, System::UnicodeString> {
   static System::UnicodeString apply(std::nullptr_t) {
      return System::UnicodeString(); // leer
      }
   };   

// Ableitungen für Delphi-Strings gehen jeweils über std::string
template <typename ty> requires (!std::same_as<ty, System::UnicodeString>)
struct Convert<ty, System::UnicodeString> {
   static System::UnicodeString apply(ty const & v) {
      return Convert<std::string, System::UnicodeString>::apply(Convert<ty, std::string>::apply(v));
      }
   };

template <typename ty> requires (!std::same_as<ty, System::AnsiString>)
struct Convert<ty, System::AnsiString> {
   static System::AnsiString apply(ty const & v) {
	  return Convert<std::string, System::AnsiString>::apply(Convert<ty, std::string>::apply(v));
      }
   };

template <typename ty> requires (!std::same_as<ty, std::wstring>)
struct Convert<ty, std::wstring> {
   static std::wstring apply(ty const & v) {
	  return Convert<std::string, std::wstring>::apply(Convert<ty, std::string>::apply(v));
      }
   };

// Strings / Zahlen für wstring/Unicode/Ansi: jeweils via std::string
template <typename To> requires (!std::same_as<To, std::wstring> && !std::same_as<To, std::string> && !std::same_as<To, System::AnsiString>)
struct Convert<System::UnicodeString, To> {
   static To apply(System::UnicodeString const & u) {
      return Convert<std::string, To>::apply(Convert<System::UnicodeString, std::string>::apply(u));
      }
   };

template <typename To> requires (!std::same_as<To, std::string>)
struct Convert<System::AnsiString, To> {
   static To apply(System::AnsiString const & a) {
	  return Convert<std::string, To>::apply(Convert<System::AnsiString, std::string>::apply(a));
      }
   };

template <typename To> requires (!std::same_as<To, std::wstring>)
struct Convert<std::wstring, To> {
   static To apply(std::wstring const & ws) {
      return Convert<std::string, To>::apply(Convert<std::wstring, std::string>::apply(ws));
      }
   };


// ==============================================================================
// ==============================================================================

// TDateTime / Ymd / Hhmmss / time_point
template <>
struct Convert<System::TDateTime, std::chrono::year_month_day> {
   static std::chrono::year_month_day apply(System::TDateTime const & dt) {
	  double daysUnix = static_cast<double>(dt) - details::kUnixEpochExcelDays;
	  std::chrono::sys_days sd = std::chrono::sys_days{std::chrono::days{static_cast<long long>(daysUnix)}};
	  return std::chrono::year_month_day{sd};
      }
   };

template <>
struct Convert<std::chrono::year_month_day, System::TDateTime> {
   static System::TDateTime apply(std::chrono::year_month_day const & ymd) {
      return System::TDateTime(details::DaysFromYmd(ymd) + details::kUnixEpochExcelDays);
      }
   };

template <>
struct Convert<std::chrono::system_clock::time_point, System::TDateTime> {
   static System::TDateTime apply(std::chrono::system_clock::time_point const & tp) {
      return System::TDateTime(details::DaysSinceUnixEpoch(tp) + details::kUnixEpochExcelDays);
      }
   };

template <>
struct Convert<System::TDateTime, std::chrono::system_clock::time_point> {
   static std::chrono::system_clock::time_point apply(System::TDateTime const & dt) {
      double daysUnix = static_cast<double>(dt) - details::kUnixEpochExcelDays;
      return details::TpFromDaysSinceUnix(daysUnix);
      }
   };


// Delphi TDateTime / Strings: ISO-Datum als Default
template <> 
struct Convert<System::TDateTime, std::string> {
   static std::string apply(System::TDateTime const & dt) {
      auto tmp = Convert<System::TDateTime, std::chrono::system_clock::time_point>::apply(dt);
      // einfache Darstellung: Tage seit 1899-12-30 als double
	  //double const serial = static_cast<double>(dt);
	  //return Convert<double, std::string>::apply(serial);
      return Convert<std::chrono::system_clock::time_point, std::string>::apply(tmp);
      }
};

template <> 
struct Convert<std::string, System::TDateTime> {
   static System::TDateTime apply(std::string const & s) {
      auto tmp = Convert<std::string, std::chrono::system_clock::time_point>::apply(s);
	  return  Convert<std::chrono::system_clock::time_point, System::TDateTime>::apply(tmp);
	  }
};



// ======================================================
// Variant / alle Typen
// ======================================================

template <typename To>
std::optional<To> ConvertToOpt(System::Variant const& from) {
   return Convert<System::Variant, To>::apply_opt(from);
}

template <typename To>
std::optional<To> ConvertSafeTo(System::Variant const& from) {
   return Convert<System::Variant, To>::apply_safe(from);
}


/// Hilfsstruktur um gegen Variant Null zu testen
struct VariantConvertSupport {
   static bool isNullVariant(System::Variant const& aValue) {
      return System::Variants::VarIsNull(aValue) || System::Variants::VarIsEmpty(aValue);
      }
   };

template <typename ty>
struct Convert<System::Variant, std::optional<ty>> : public VariantConvertSupport {
   static_assert(!adecc::is_optional_v<ty>, "Nested std::optional is not supported");

   using to_ty = std::optional<ty>;

   static to_ty apply(System::Variant const& aValue) {
      if (isNullVariant(aValue)) {
         return std::nullopt;
         }
      return std::optional<ty>{ Convert<System::Variant, ty>::apply(aValue) };
      }
};
   
template <typename Derived>
struct VarOptBase : public VariantConvertSupport {
   
   static auto apply_safe(System::Variant const& v) {
	  if (isNullVariant(v)) throw std::invalid_argument("Convert fails, Parameter is Null");
	  else [[likely]] return Derived::apply(v);
	  }

   static auto apply_opt(System::Variant const& v) {
      using to_ty = typename Derived::to_ty;
	  if (isNullVariant(v)) {
		 return std::optional<to_ty>{};
		 }
	  else [[likely]] return std::optional<to_ty>{ Derived::apply(v) };
	  }
   };

template <>
struct Convert<System::Variant, std::string> : public VarOptBase<Convert<System::Variant, std::string>> {
   using to_ty = std::string;
   static std::string apply(System::Variant const & v) {
	  return Convert<System::UnicodeString, std::string>::apply(System::Variants::VarToStr(v));
	  }
};

template <>
struct Convert<System::Variant, System::UnicodeString> : public VarOptBase<Convert<System::Variant, System::UnicodeString>> {
   using to_ty = System::UnicodeString;
   static System::UnicodeString apply(System::Variant const & v) {
      return System::Variants::VarToStr(v);
	  }
};

template <>
struct Convert<System::Variant, System::AnsiString> : public VarOptBase<Convert<System::Variant, System::AnsiString>> {
   using to_ty = System::AnsiString;
   static System::AnsiString apply(System::Variant const & v) {
	  return System::AnsiString(System::Variants::VarToStr(v));
	  }
};

template <> struct Convert<System::Variant, std::wstring> : public VarOptBase<Convert<System::Variant, std::wstring>> {
   using to_ty = std::wstring;
   static std::wstring apply(System::Variant const & v) {
	  return Convert<System::UnicodeString, std::wstring>::apply(System::Variants::VarToStr(v));
   }
};


template <Integral I>
struct Convert<System::Variant, I>  : public VarOptBase<Convert<System::Variant, I>> {
   using to_ty = I;
   static I apply(System::Variant const & v) {
	  auto vv = System::Variants::VarAsType(v, sizeof(I) >= 8 ? System::varInt64 : System::varInteger);
	  return static_cast<I>(static_cast<long long>(vv));
   }
};

template <Floating F>
struct Convert<System::Variant, F>  : public VarOptBase<Convert<System::Variant, F>> {
   using to_ty = F;
   static F apply(System::Variant const & v) {
	  auto vv = System::Variants::VarAsType(v, System::varDouble);
	  return static_cast<F>(static_cast<double>(vv));
   }
};

template<std::floating_point ty, std::size_t uDecimals, typename TRoundingPolicy>
struct Convert<System::Variant, adecc::numeric<ty, uDecimals, TRoundingPolicy>>
   : public VarOptBase<Convert<System::Variant, adecc::numeric<ty, uDecimals, TRoundingPolicy>>> {
   using to_ty = adecc::numeric<ty, uDecimals, TRoundingPolicy>;

   static to_ty apply(System::Variant const& v) {
      auto vv = System::Variants::VarAsType(v, System::varDouble);
      return to_ty {static_cast<ty>(static_cast<double>(vv))};
      }
};

template <>
struct Convert<System::Variant, bool> : public VarOptBase<Convert<System::Variant, bool>> {
   using to_ty = bool;
   static bool apply(System::Variant const& v) {
	  System::Variant vb = System::Variants::VarAsType(v, System::varBoolean);
	  short w = static_cast<short>(vb);
	  return w != 0;
	  }
};

template <>
struct Convert<System::Variant, System::TDateTime> : public VarOptBase<Convert<System::Variant, System::TDateTime>> {
   using to_ty = System::TDateTime;
   static System::TDateTime apply(System::Variant const & v) {
	  return static_cast<System::TDateTime>(System::Variants::VarAsType(v, System::varDate));
	  }
};

template <>
struct Convert<System::Variant, std::chrono::system_clock::time_point>  : public VarOptBase<Convert<System::Variant, std::chrono::system_clock::time_point>> {
   using to_ty = std::chrono::system_clock::time_point;
   static std::chrono::system_clock::time_point apply(System::Variant const & v) {
	  return Convert<System::TDateTime, std::chrono::system_clock::time_point>::apply(Convert<System::Variant, System::TDateTime>::apply(v));
   }
};

template <>
struct Convert<System::Variant, std::chrono::year_month_day> : public VarOptBase<Convert<System::Variant, std::chrono::year_month_day>> {
   using to_ty = std::chrono::year_month_day;
   static std::chrono::year_month_day apply(System::Variant const & v) {
	  return Convert<System::TDateTime, std::chrono::year_month_day>::apply(Convert<System::Variant, System::TDateTime>::apply(v));
   }
};

template <>
struct Convert<System::Variant, std::chrono::hh_mm_ss<std::chrono::seconds>> : public VarOptBase<Convert<System::Variant, std::chrono::hh_mm_ss<std::chrono::seconds>>> {
   using to_ty = std::chrono::hh_mm_ss<std::chrono::seconds>;
   static std::chrono::hh_mm_ss<std::chrono::seconds> apply(System::Variant const & v) {
	  auto dt = Convert<System::Variant, System::TDateTime>::apply(v);
	  double frac = static_cast<double>(dt) - std::floor(static_cast<double>(dt));
	  using namespace std::chrono;
	  auto secs = seconds{static_cast<long long>(frac * details::kSecPerDay)};
	  return std::chrono::hh_mm_ss<std::chrono::seconds>{secs};
   }
};

// alle unterstützten Typen / Variant (Delegation auf geeignete Variant-Konstruktoren)
template <>
struct Convert<std::string, System::Variant> {
   static System::Variant apply(std::string const & s) {
	  return System::Variant(Convert<std::string, System::UnicodeString>::apply(s));
   }
};


template<>
struct Convert<std::string_view, System::Variant> {
   static System::Variant apply(std::string_view sv) {
      return System::Variant(::System::UnicodeString(sv.data(), int(sv.size())));
   }
};

template<>
struct Convert<char const*, System::Variant> {
   static System::Variant apply(char const* p) {
      return System::Variant(::System::UnicodeString(p ? p : ""));
   }
};

template <>
struct Convert<std::wstring, System::Variant> {
   static System::Variant apply(std::wstring const & ws) {
	  return System::Variant(Convert<std::wstring, System::UnicodeString>::apply(ws));
   }
};

template <>
struct Convert<System::UnicodeString, System::Variant> {
   static System::Variant apply(System::UnicodeString const & u) { return System::Variant(u); }
};

template <>
struct Convert<System::AnsiString, System::Variant> {
   static System::Variant apply(System::AnsiString const & a) { return System::Variant(a); }
};

template <>
struct Convert<bool, System::Variant> {
   static System::Variant apply(bool const b) { return System::Variant(b); }
};

template <Integral I>
struct Convert<I, System::Variant> {
   static System::Variant apply(I const v) { return System::Variant(static_cast<long long>(v)); }
};

template <Floating F>
struct Convert<F, System::Variant> {
   static System::Variant apply(F const v) { return System::Variant(static_cast<double>(v)); }
   };

template<std::floating_point ty, std::size_t uDecimals, typename TRoundingPolicy>
struct Convert<adecc::numeric<ty, uDecimals, TRoundingPolicy>, System::Variant>
   : public VarOptBase<Convert<adecc::numeric<ty, uDecimals, TRoundingPolicy>, System::Variant>> {
   using from_ty = adecc::numeric<ty, uDecimals, TRoundingPolicy>;
   using to_ty = System::Variant;

   static to_ty apply(from_ty const& aValue) {
      return System::Variant {static_cast<double>(aValue.Get())};
      }
};

   
template <>
struct Convert<System::TDateTime, System::Variant> {
   static System::Variant apply(System::TDateTime const & dt) { return System::Variant(dt); }
   };

template <>
struct Convert<std::chrono::system_clock::time_point, System::Variant> {
   static System::Variant apply(std::chrono::system_clock::time_point const & tp) { return Convert<System::TDateTime, System::Variant>::apply(Convert<std::chrono::system_clock::time_point, System::TDateTime>::apply(tp)); }
   };

template <>
struct Convert<std::chrono::year_month_day, System::Variant> {
   static System::Variant apply(std::chrono::year_month_day const & ymd) { return Convert<System::TDateTime, System::Variant>::apply(Convert<std::chrono::year_month_day, System::TDateTime>::apply(ymd)); }
   };

template <>
struct Convert<std::chrono::hh_mm_ss<std::chrono::seconds>, System::Variant> {
   static System::Variant apply(std::chrono::hh_mm_ss<std::chrono::seconds> const & hms) {
	  using namespace std::chrono;
	  auto secs = duration_cast<seconds>(hms.to_duration()).count();
	  double frac = static_cast<double>(secs) / details::kSecPerDay;
	  return System::Variant(System::TDateTime(frac));
      }
   };


template <class ty> requires requires(ty const& aValue) {
        { Convert<ty, System::Variant>::apply(aValue) } -> std::same_as<System::Variant>;
       }
struct Convert<std::optional<ty>, System::Variant> {
   static System::Variant apply(std::optional<ty> const& val) {
      if(!val) {
         return System::Variants::Null();
         }
      return Convert<ty, System::Variant>::apply(*val);
      }
   };

   
} // end of namepace adecc

