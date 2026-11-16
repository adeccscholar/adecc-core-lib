#pragma once

#include "value_types.h"

#include "delphi_UI_core.h"
#include "delphi_convert.h"

#include <System.hpp>

namespace adecc::delphi {



// ---------------------------------------------
// Backend-Enum
// ---------------------------------------------
enum class Framework: int32_t { vcl, fmx };

// ---------------------------------------------
// Includes + Traits pro Backend
// ---------------------------------------------
template <Framework> struct GridTraits;     // Primärtemplate (nicht definiert)

#if defined(BUILD_WITH_VCL)
  #include <Vcl.Grids.hpp>

  template <> struct GridTraits<Framework::vcl> {
     using GridT = ::TStringGrid;
     static constexpr int header_row_offset = 1; // Header in Row 0, Daten ab 1
  };
#endif

#if defined(BUILD_WITH_FMX)
  #include <FMX.Grid.hpp>

  template <> struct GridTraits<Framework::fmx> {
     using GridT = ::TStringGrid;
     static constexpr int header_row_offset = 0; // Header in Columns, Daten ab 0
  };
#endif

// Kompiler-Schutz bei fehlenden Headers
#if !defined(BUILD_WITH_VCL) && !defined(BUILD_WITH_FMX)
  #error Define BUILD_WITH_VCL or BUILD_WITH_FMX before including this header.
#endif


template <Framework B, StreamPolicy ty>
class StringGridWrapper {
   using Traits = GridTraits<B>;
   using GridT  = typename Traits::GridT;

public:
   explicit StringGridWrapper(GridT* grid, vecCaptions<ty> const& headers, bool clear = true) 
              : grid_{grid}, cols_{(int)headers.size()}, header_row_offset_{Traits::header_row_offset} {
      static_assert(std::is_pointer_v<GridT*>, "Grid type must be pointer to TStringGrid");
      if(!grid_) return;

      // Lambda: Spalten initialisieren (unterscheidet VCL / FMX)
      auto setup_columns = [&] {
         if constexpr (B == Framework::vcl) {
            if(clear) {
               grid_->ColCount = cols_;
               grid_->RowCount = 1; // Kopfzeile
               for(int c=0; c < cols_; ++c) {
                  auto const& [name, w, align] = headers[(size_t)c];
                  grid_->Cells[c][0] = adecc::ConvertTo<System::UnicodeString>(name);
                  if(w > 0) grid_->ColWidths[c] = w;
                  (void)align; // per-column alignment erfordert OwnerDraw -> erst einmal ausgelassen
                  }
               }
            } 
         else { // FMX
            grid_->ReadOnly = true;
            grid_->Options <<= TGridOption::RowSelect;
            grid_->Options >>= TGridOption::ColumnMove;
            grid_->Options <<= TGridOption::AlwaysShowSelection;            
            if(clear) {
               grid_->BeginUpdate();
               try {
                  grid_->ClearColumns();
                  for(auto const& [name, w, align] : headers) {
                     auto* col = new TStringColumn(grid_);
                     col->Header = adecc::ConvertTo<System::UnicodeString>(name);
                     if(w > 0) col->Width = w;
                     switch(align) {
                        case EAlignmentType::left:   col->HorzAlign = TTextAlign::Leading;  break;
                        case EAlignmentType::right:  col->HorzAlign = TTextAlign::Trailing; break;
                        case EAlignmentType::center: col->HorzAlign = TTextAlign::Center;   break;
                        default:                     col->HorzAlign = TTextAlign::Leading;  break;
                        }
                     grid_->AddObject(col);
                     }
                  grid_->RowCount = 0;
                  grid_->EndUpdate();
                  } 
               catch(...) { 
                  grid_->EndUpdate(); 
                  throw; 
                  }
               }
            }
         };
      setup_columns();
      }


   // ---------------------------------------------------------
   // Eine Zeile aus einem std::tuple anhängen (operator+=)
   // ---------------------------------------------------------
   template <typename... Ts>
   StringGridWrapper& operator += (std::tuple<Ts...> const& row) {
      appendTuple_(row, std::make_index_sequence<sizeof...(Ts)>{});
      return *this;
      }

   // Bequemer Helfer für variadische Werte
   template <typename... Ts>
   StringGridWrapper& add(Ts&&... values) {
      auto tup = std::make_tuple(std::forward<Ts>(values)...);
      return (*this += tup);
      }



   // -----------------------------------------------------------------
   // 1) Variadic nur mit Typen: ordnet die ersten N Spalten zu
   //    get_row<Ts...>(row)  -> tuple<Ts...>   (Spalten 0..N-1)
   // -----------------------------------------------------------------
   template <typename... Ts>
   [[nodiscard]] std::tuple<Ts...> get_row(std::size_t row) const {
      if(!grid_) throw std::runtime_error("get_row: grid is null");

      const int phys_row = [&,this] {
         if constexpr (B == Framework::vcl) {
            const int r = static_cast<int>(row) + header_row_offset_;
            if (r < 0 || r >= grid_->RowCount) throw std::runtime_error("get_row: row out of range");
            return r;
            } 
         else {
            const int r = static_cast<int>(row);
            if (r < 0 || r >= grid_->RowCount) throw std::runtime_error("get_row: row out of range");
            return r;
            }
         }();

      constexpr std::size_t N = sizeof...(Ts);
      if (static_cast<int>(N) > cols_)
         throw std::runtime_error("get_row: requested more columns than available");

      auto get_cell = [&](int c) -> System::UnicodeString {
         if (c < 0 || c >= cols_) throw std::runtime_error("get_row: column out of range");
         if constexpr (B == Framework::vcl) return grid_->Cells[c][phys_row];
         else                               return grid_->Cells[c][phys_row];
         };

      // baue Tuple aus Spalten 0..N-1
      return [&]<std::size_t... I>(std::index_sequence<I...>) {
         return std::tuple<Ts...>{ adecc::ConvertTo<Ts>(get_cell(static_cast<int>(I)))... };
         }(std::make_index_sequence<N>{});
      }

   // -----------------------------------------------------------------
   // 2) Mit Spaltenliste: Spalten[i] -> Typ Ti
   //    get_row_cols<Ts...>(row, cols) -> tuple<Ts...>
   // -----------------------------------------------------------------
   template <typename... Ts>
   [[nodiscard]] std::tuple<Ts...> get_row_cols(std::size_t row, std::vector<std::size_t> const& cols) const {
      if(!grid_) throw std::runtime_error("get_row_cols: grid is null");
      if (cols.size() != sizeof...(Ts))
         throw std::runtime_error("get_row_cols: size mismatch between types and cols");

      const int phys_row = [&,this] {
         if constexpr (B == Framework::vcl) {
            const int r = static_cast<int>(row) + header_row_offset_;
            if (r < 0 || r >= grid_->RowCount) throw std::runtime_error("get_row_cols: row out of range");
            return r;
            } 
         else {
            const int r = static_cast<int>(row);
            if (r < 0 || r >= grid_->RowCount) throw std::runtime_error("get_row_cols: row out of range");
            return r;
            }
         }();

      auto get_cell = [&](std::size_t idx) -> System::UnicodeString {
         if (idx >= static_cast<std::size_t>(cols_)) throw std::runtime_error("get_row_cols: column out of range");
         int c = static_cast<int>(idx);
         if constexpr (B == Framework::vcl) return grid_->Cells[c][phys_row];
         else                               return grid_->Cells[c][phys_row];
         };

      // entpacke cols in ein tuple<Ts...>
      // Trick: wir kopieren die Spaltenindizes lokal in ein array, um in Index-Sequenz zuzugreifen
      std::array<std::size_t, sizeof...(Ts)> arr{};
      std::copy(cols.begin(), cols.end(), arr.begin());

      return [&]<std::size_t... I>(std::index_sequence<I...>) {
         return std::tuple<Ts...>{ ConvertTo<Ts>( get_cell(arr[I]) )... };
         }(std::make_index_sequence<sizeof...(Ts)>{});
      }

   // Bequeme Kurzformen für letzte Datenzeile:
   template <typename... Ts>
   [[nodiscard]] std::tuple<Ts...> get_last() const {
      if (current_row_ <= 0) throw std::runtime_error("get_last: no data row");
      return get_row<Ts...>(static_cast<std::size_t>(current_row_ - 1));
      }

   template <typename... Ts>
   [[nodiscard]] std::tuple<Ts...> get_last_cols(std::vector<std::size_t> const& cols) const {
      if (current_row_ <= 0) throw std::runtime_error("get_last_cols: no data row");
      return get_row_cols<Ts...>(static_cast<std::size_t>(current_row_ - 1), cols);
   }






   int columns()   const noexcept { return cols_; }
   int data_rows() const noexcept { return current_row_; }


private:
   template <typename Tuple, std::size_t... I>
   void appendTuple_(Tuple const& row, std::index_sequence<I...>) {
      if(!grid_) return;

      constexpr int N = (int)sizeof...(I);
      int const use = (N < cols_ ? N : cols_);

      // Lambda: sicherstellen, dass die Zielzeile existiert (unterscheidet VCL/FMX)
      auto ensure_row = [&] {
         if constexpr (B == Framework::vcl) {
            if(grid_->RowCount < current_row_ + header_row_offset_ + 1) {
               grid_->RowCount = current_row_ + header_row_offset_ + 1;
               }
            } 
         else { // FMX
            if(grid_->RowCount < current_row_ + 1) {
               grid_->RowCount = current_row_ + 1;
               }
            }
         };

      // Lambda: Zelle setzen (unterscheidet VCL/FMX)
      auto set_cell = [&](int col, System::UnicodeString const& u) {
         if constexpr (B == Framework::vcl) {
            grid_->Cells[col][current_row_ + header_row_offset_] = u;
            } 
         else { // FMX
            grid_->Cells[col][current_row_] = u;
            }
         };

      ensure_row();
      // Falt-Ausdruck: schreibe Spalten 0..use-1
      ( (I < (size_t)use ? set_cell((int)I, adecc::ConvertTo<System::UnicodeString>(std::get<I>(row))) : void()), ... );
      ++current_row_;
      }

private:
   GridT* grid_ = nullptr;
   int    cols_ = 0;
   int    current_row_ = 0;
   int    header_row_offset_ = 0;
};

/*
// in StringGridWrapper<B> (public) ergänzen:
public:
   // Setze Spalten 0..N-1 aus einem Tupel (schreibend)
   template <typename... Ts>
   void set_row(std::size_t row, std::tuple<Ts...> const& t) {
      if(!grid_) throw std::runtime_error("set_row: grid is null");
      const int phys_row = [&,this]{
         if constexpr (B == GridBackend::vcl) {
            int r = (int)row + header_row_offset_;
            if(r < 0 || r >= grid_->RowCount) throw std::runtime_error("set_row: row out of range");
            return r;
         } else {
            int r = (int)row;
            if(r < 0 || r >= grid_->RowCount) throw std::runtime_error("set_row: row out of range");
            return r;
         }
      }();

      constexpr std::size_t N = sizeof...(Ts);
      if ((int)N > cols_) throw std::runtime_error("set_row: more values than columns");

      auto set_cell = [&](int c, System::UnicodeString const& u) {
         if constexpr (B == GridBackend::vcl) grid_->Cells[c][phys_row] = u;
         else                                  grid_->Cells[c][phys_row] = u;
      };
      [&]<std::size_t... I>(std::index_sequence<I...>){
         ( set_cell((int)I, detail::toU(std::get<I>(t))), ... );
      }(std::make_index_sequence<N>{});
   }

   // Setze Werte in beliebigen Spalten (Spaltenliste[i] ← Tuple[i])
   template <typename... Ts>
   void set_row_cols(std::size_t row,
                     std::vector<std::size_t> const& cols,
                     std::tuple<Ts...> const& t)
   {
      if(!grid_) throw std::runtime_error("set_row_cols: grid is null");
      if(cols.size() != sizeof...(Ts)) throw std::runtime_error("set_row_cols: size mismatch");

      const int phys_row = [&,this]{
         if constexpr (B == GridBackend::vcl) {
            int r = (int)row + header_row_offset_;
            if(r < 0 || r >= grid_->RowCount) throw std::runtime_error("set_row_cols: row out of range");
            return r;
         } else {
            int r = (int)row;
            if(r < 0 || r >= grid_->RowCount) throw std::runtime_error("set_row_cols: row out of range");
            return r;
         }
      }();

      std::array<std::size_t, sizeof...(Ts)> arr{};
      std::copy(cols.begin(), cols.end(), arr.begin());

      auto set_cell = [&](int c, System::UnicodeString const& u) {
         if(c < 0 || c >= cols_) throw std::runtime_error("set_row_cols: column out of range");
         if constexpr (B == GridBackend::vcl) grid_->Cells[c][phys_row] = u;
         else                                  grid_->Cells[c][phys_row] = u;
      };

      [&]<std::size_t... I>(std::index_sequence<I...>){
         ( set_cell((int)arr[I], detail::toU(std::get<I>(t))), ... );
      }(std::make_index_sequence<sizeof...(Ts)>{});
   }

   // Datenzeilen-Anzahl als Grundlage für die Views
   int data_row_count() const noexcept { return current_row_; }



namespace ranges_grid {

// -----------------------------------------------------------------------------
// Random-Access Proxy/Iterator/View – über ALLE Spalten (0..N-1) für Ts...
// -----------------------------------------------------------------------------
template <auto> class StringGridWrapper; // forward nur, falls Header getrennt

template <GridBackend B, typename... Ts>
class grid_row_proxy {
   using Wrapper = StringGridWrapper<B>;
public:
   grid_row_proxy() = default;
   grid_row_proxy(Wrapper* w, std::size_t r) : w_{w}, row_{r} {}

   // Lesen
   operator std::tuple<Ts...>() const {
      return w_->template get_row<Ts...>(row_);
   }
   // Schreiben
   grid_row_proxy& operator=(std::tuple<Ts...> const& t) {
      w_->template set_row<Ts...>(row_, t);
      return *this;
   }
private:
   Wrapper*     w_   = nullptr;
   std::size_t  row_ = 0;
};

template <GridBackend B, typename... Ts>
class grid_row_iterator {
   using Wrapper = StringGridWrapper<B>;
public:
   using iterator_concept  = std::random_access_iterator_tag;
   using iterator_category = std::random_access_iterator_tag;
   using difference_type   = std::ptrdiff_t;
   using value_type        = std::tuple<Ts...>;
   using reference         = grid_row_proxy<B, Ts...>;

   grid_row_iterator() = default;
   grid_row_iterator(Wrapper* w, std::size_t r) : w_{w}, row_{r} {}

   reference   operator*()  const { return reference{w_, row_}; }
   value_type  operator[](difference_type n) const { return reference{w_, row_ + (std::size_t)n}; }

   grid_row_iterator& operator++()    { ++row_; return *this; }
   grid_row_iterator  operator++(int) { auto tmp=*this; ++*this; return tmp; }
   grid_row_iterator& operator--()    { --row_; return *this; }
   grid_row_iterator  operator--(int) { auto tmp=*this; --*this; return tmp; }

   grid_row_iterator& operator+=(difference_type n) { row_ += (std::size_t)n; return *this; }
   grid_row_iterator& operator-=(difference_type n) { row_ -= (std::size_t)n; return *this; }

   friend grid_row_iterator operator+(grid_row_iterator it, difference_type n){ it+=n; return it; }
   friend grid_row_iterator operator+(difference_type n, grid_row_iterator it){ it+=n; return it; }
   friend grid_row_iterator operator-(grid_row_iterator it, difference_type n){ it-=n; return it; }
   friend difference_type   operator-(grid_row_iterator a, grid_row_iterator b){
      return (difference_type)a.row_ - (difference_type)b.row_;
   }

   friend bool operator==(grid_row_iterator a, grid_row_iterator b){ return a.w_==b.w_ && a.row_==b.row_; }
   friend auto operator<=>(grid_row_iterator const& a, grid_row_iterator const& b){
      if (a.w_ != b.w_) return a.w_ < b.w_ ? std::strong_ordering::less : std::strong_ordering::greater;
      return (a.row_ < b.row_) ? std::strong_ordering::less :
             (a.row_ > b.row_) ? std::strong_ordering::greater : std::strong_ordering::equal;
   }
private:
   Wrapper*    w_   = nullptr;
   std::size_t row_ = 0;
};

template <GridBackend B, typename... Ts>
class grid_row_view : public std::ranges::view_interface<grid_row_view<B, Ts...>> {
   using Wrapper  = StringGridWrapper<B>;
   using iterator = grid_row_iterator<B, Ts...>;
public:
   grid_row_view() = default;
   explicit grid_row_view(Wrapper* w) : w_{w} {}

   iterator begin() const { return iterator{w_, 0}; }
   iterator end()   const { return iterator{w_, (std::size_t)w_->data_row_count()}; }

   std::size_t size() const { return (std::size_t)w_->data_row_count(); }

   // Optionale Komfort-Funktion: Random access
   value_type operator[](std::size_t i) const { return *begin() + (std::ptrdiff_t)i; }

private:
   Wrapper* w_ = nullptr;
};

// -----------------------------------------------------------------------------
// Random-Access Proxy/Iterator/View – mit SPALTENLISTE für Ts...
// -----------------------------------------------------------------------------
template <GridBackend B, typename... Ts>
class grid_row_cols_proxy {
   using Wrapper = StringGridWrapper<B>;
public:
   grid_row_cols_proxy() = default;
   grid_row_cols_proxy(Wrapper* w, std::size_t r, std::span<const std::size_t> cols)
     : w_{w}, row_{r}, cols_{cols} {}

   operator std::tuple<Ts...>() const {
      std::vector<std::size_t> v(cols_.begin(), cols_.end());
      return w_->template get_row_cols<Ts...>(row_, v);
   }
   grid_row_cols_proxy& operator=(std::tuple<Ts...> const& t) {
      std::vector<std::size_t> v(cols_.begin(), cols_.end());
      w_->template set_row_cols<Ts...>(row_, v, t);
      return *this;
   }
private:
   Wrapper*                       w_    = nullptr;
   std::size_t                    row_  = 0;
   std::span<const std::size_t>   cols_{};
};

template <GridBackend B, typename... Ts>
class grid_row_cols_iterator {
   using Wrapper = StringGridWrapper<B>;
public:
   using iterator_concept  = std::random_access_iterator_tag;
   using iterator_category = std::random_access_iterator_tag;
   using difference_type   = std::ptrdiff_t;
   using value_type        = std::tuple<Ts...>;
   using reference         = grid_row_cols_proxy<B, Ts...>;

   grid_row_cols_iterator() = default;
   grid_row_cols_iterator(Wrapper* w, std::size_t r, std::span<const std::size_t> cols)
      : w_{w}, row_{r}, cols_{cols} {}

   reference   operator*()  const { return reference{w_, row_, cols_}; }
   value_type  operator[](difference_type n) const { return reference{w_, row_ + (std::size_t)n, cols_}; }

   grid_row_cols_iterator& operator++()    { ++row_; return *this; }
   grid_row_cols_iterator  operator++(int) { auto tmp=*this; ++*this; return tmp; }
   grid_row_cols_iterator& operator--()    { --row_; return *this; }
   grid_row_cols_iterator  operator--(int) { auto tmp=*this; --*this; return tmp; }

   grid_row_cols_iterator& operator+=(difference_type n) { row_ += (std::size_t)n; return *this; }
   grid_row_cols_iterator& operator-=(difference_type n) { row_ -= (std::size_t)n; return *this; }

   friend grid_row_cols_iterator operator+(grid_row_cols_iterator it, difference_type n){ it+=n; return it; }
   friend grid_row_cols_iterator operator+(difference_type n, grid_row_cols_iterator it){ it+=n; return it; }
   friend grid_row_cols_iterator operator-(grid_row_cols_iterator it, difference_type n){ it-=n; return it; }
   friend difference_type operator-(grid_row_cols_iterator a, grid_row_cols_iterator b){
      return (difference_type)a.row_ - (difference_type)b.row_;
   }

   friend bool operator==(grid_row_cols_iterator a, grid_row_cols_iterator b){
      return a.w_==b.w_ && a.row_==b.row_ &&
             a.cols_.data()==b.cols_.data() && a.cols_.size()==b.cols_.size();
   }
   friend auto operator<=>(grid_row_cols_iterator const& a, grid_row_cols_iterator const& b){
      if (a.w_ != b.w_) return a.w_ < b.w_ ? std::strong_ordering::less : std::strong_ordering::greater;
      return (a.row_ < b.row_) ? std::strong_ordering::less :
             (a.row_ > b.row_) ? std::strong_ordering::greater : std::strong_ordering::equal;
   }
private:
   Wrapper*                     w_    = nullptr;
   std::size_t                  row_  = 0;
   std::span<const std::size_t> cols_{};
};

template <GridBackend B, typename... Ts>
class grid_row_cols_view : public std::ranges::view_interface<grid_row_cols_view<B, Ts...>> {
   using Wrapper  = StringGridWrapper<B>;
   using iterator = grid_row_cols_iterator<B, Ts...>;
public:
   grid_row_cols_view() = default;
   grid_row_cols_view(Wrapper* w, std::vector<std::size_t> cols)
      : w_{w}, holder_{std::move(cols)} {}

   iterator begin() const { return iterator{w_, 0, std::span<const std::size_t>(holder_)}; }
   iterator end()   const { return iterator{w_, (std::size_t)w_->data_row_count(), std::span<const std::size_t>(holder_)}; }

   std::size_t size() const { return (std::size_t)w_->data_row_count(); }

private:
   Wrapper*                w_      = nullptr;
   std::vector<std::size_t> holder_; // hält die Spaltenliste am Leben
};

// -----------------------------------------------------------------------------
// Fabriken (bequem in Pipelines nutzbar)
// -----------------------------------------------------------------------------
template <GridBackend B, typename... Ts>
[[nodiscard]] inline auto make_row_view(StringGridWrapper<B>& wrap) {
   return grid_row_view<B, Ts...>{ &wrap };
}

template <GridBackend B, typename... Ts>
[[nodiscard]] inline auto make_row_view_cols(StringGridWrapper<B>& wrap,
                                             std::vector<std::size_t> cols) {
   return grid_row_cols_view<B, Ts...>{ &wrap, std::move(cols) };
}

} // namespace ranges_grid

// -----------------------------------------------------------------------------
// Ranges-Customization: borrowed & view
// (Spezialisierungen sind in std::ranges erlaubt)
// -----------------------------------------------------------------------------
namespace std::ranges {

// Beide Views sind *borrowed*, weil sie nur einen Zeiger auf den langlebigen Wrapper halten.
// Damit dürfen sie in Pipelines aus lvalue-Quellen weitergereicht werden.
template <GridBackend B, typename... Ts>
inline constexpr bool enable_borrowed_range<ranges_grid::grid_row_view<B, Ts...>> = true;

template <GridBackend B, typename... Ts>
inline constexpr bool enable_borrowed_range<ranges_grid::grid_row_cols_view<B, Ts...>> = true;

// Einige Toolchains profitieren zusätzlich von enable_view=true
template <GridBackend B, typename... Ts>
inline constexpr bool enable_view<ranges_grid::grid_row_view<B, Ts...>> = true;

template <GridBackend B, typename... Ts>
inline constexpr bool enable_view<ranges_grid::grid_row_cols_view<B, Ts...>> = true;

} // namespace std::ranges


// neue Zeile anhängen aus Tupel (Spalten 0..N-1)
template <typename... Ts>
void append_row(std::tuple<Ts...> const& t) {
   if(!grid_) throw std::runtime_error("append_row: grid is null");
   constexpr std::size_t N = sizeof...(Ts);
   if ((int)N > cols_) throw std::runtime_error("append_row: too many elements");

   // index der neuen Datenzeile
   std::size_t row = static_cast<std::size_t>(current_row_);
   // physische Zeile bereitstellen
   if constexpr (B == GridBackend::vcl) {
      int phys = static_cast<int>(row) + header_row_offset_;
      if (grid_->RowCount < phys + 1) grid_->RowCount = phys + 1;
      // Zellen schreiben
      [&]<std::size_t... I>(std::index_sequence<I...>) {
         ( (grid_->Cells[(int)I][phys] = detail::toU(std::get<I>(t))), ... );
      }(std::make_index_sequence<N>{});
   } else { // FMX
      int phys = static_cast<int>(row);
      if (grid_->RowCount < phys + 1) grid_->RowCount = phys + 1;
      [&]<std::size_t... I>(std::index_sequence<I...>) {
         ( (grid_->Cells[(int)I][phys] = detail::toU(std::get<I>(t))), ... );
      }(std::make_index_sequence<N>{});
   }

   template <typename... Ts>
void append(Ts&&... vs) {
   append_row(std::tuple<std::decay_t<Ts>...>{ std::forward<Ts>(vs)... });
}


 namespace ranges_grid {

template <GridBackend B, typename... Ts>
class grid_row_back_inserter {
   using Wrapper = StringGridWrapper<B>;
public:
   using iterator_concept = std::output_iterator_tag;
   using difference_type  = void;
   using value_type       = void;
   using reference        = void;
   using pointer          = void;

   explicit grid_row_back_inserter(Wrapper* w) : w_{w} {}

   // Zuweisung eines Tuples hängt neue Zeile an
   grid_row_back_inserter& operator=(std::tuple<Ts...> const& t) {
      w_->template append_row<Ts...>(t);
      return *this;
   }

   // optional: Zuweisung aus beliebigem Typ, der in tuple<Ts...> wandelbar ist
   template <typename U>
   requires requires(U const& u) { std::tuple<Ts...>{u}; }
   grid_row_back_inserter& operator=(U const& u) {
      std::tuple<Ts...> t{u};
      w_->template append_row<Ts...>(t);
      return *this;
   }

   grid_row_back_inserter& operator*()     { return *this; }
   grid_row_back_inserter& operator++()    { return *this; }
   grid_row_back_inserter& operator++(int) { return *this; }

private:
   Wrapper* w_ = nullptr;
};

// Fabrik
template <GridBackend B, typename... Ts>
[[nodiscard]] inline grid_row_back_inserter<B, Ts...>
make_row_back_inserter(StringGridWrapper<B>& wrap) {
   return grid_row_back_inserter<B, Ts...>{ &wrap };
}

} // namespace ranges_grid

*/

} // end of namespace adecc::delphi

