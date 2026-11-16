#pragma once

#include "delphi_concepts.h"

#include <System.hpp>  

#include <string>
#include <vector>
#include <iterator>
#include <ranges>
#include <compare>
#include <cstddef>

namespace adecc::delphi {

template <HasLinesOrItems owner_ty>
class LinesView {
public:
   using difference_type = std::ptrdiff_t;
   using size_type       = std::size_t;

   class LineRef {
   public:
      LineRef() = default;
      LineRef(System::Classes::TStrings* const aLines, int const iIndex) : aLines{aLines}, iIndex{iIndex} {}

      LineRef(LineRef const&)            = default;
      LineRef(LineRef&&)                 = default;
      LineRef& operator=(LineRef const&) = default;
      LineRef& operator=(LineRef&&)      = default;

      operator std::string() const {
         return ConvertTo<std::string>(aLines->Strings[iIndex]);
         }
         
      LineRef& operator = (std::string const& v) { 
         aLines->Strings[iIndex] = ConvertTo<System::UnicodeString>(v); 
         return *this; 
         }
         
      LineRef& operator = (char const* s)        { 
         aLines->Strings[iIndex] = ConvertTo<System::UnicodeString>(s); 
         return *this; 
         }
         
      LineRef& operator+=(std::string const& suf) {
         System::UnicodeString u = aLines->Strings[iIndex];
         u += suf.c_str();
         aLines->Strings[iIndex] = u;
         return *this;
         }
         
      auto operator <=> (LineRef const& other) const {
         return static_cast<std::string>(*this) <=> static_cast<std::string>(other);
         }
 
   private:
      System::Classes::TStrings* aLines{};
      int iIndex{};
   };

   class iterator {
   public:
      using iterator_concept  = std::random_access_iterator_tag;
      using iterator_category = std::random_access_iterator_tag;
      using value_type        = std::string;
      using difference_type   = std::ptrdiff_t;
      using reference         = LineRef;

      iterator() = default;
      iterator(System::Classes::TStrings* const a, int const i) : aLines{a}, iIndex{i} {}

      LineRef& operator* () const { theRef = LineRef{aLines, iIndex}; return theRef; }
      LineRef& operator[](difference_type n) const { theRef = LineRef{aLines, (int)(iIndex+n)}; return theRef; }

      iterator& operator++() { ++iIndex; return *this; }
      iterator  operator++(int) { auto t=*this; ++(*this); return t; }
      iterator& operator--() { --iIndex; return *this; }
      iterator  operator--(int) { auto t=*this; --(*this); return t; }

      iterator& operator += (difference_type n){ iIndex += (int)n; return *this; }
      iterator& operator -= (difference_type n){ iIndex -= (int)n; return *this; }
      friend iterator operator + (iterator it, difference_type n){ it+=n; return it; }
      friend iterator operator + (difference_type n, iterator it){ it+=n; return it; }
      friend iterator operator - (iterator it, difference_type n){ it-=n; return it; }

      friend difference_type operator-(iterator const& a, iterator const& b){
         return (difference_type)a.iIndex - (difference_type)b.iIndex;
         }
         
      friend bool operator==(iterator const& a, iterator const& b){
         return a.aLines==b.aLines && a.iIndex==b.iIndex;
         }
         
      friend std::strong_ordering operator<=>(iterator const& a, iterator const& b){
         if(a.aLines!=b.aLines) return std::compare_three_way()(a.aLines, b.aLines);
         return a.iIndex <=> b.iIndex;
         }

      friend std::string iter_move(iterator const it) noexcept {
         return static_cast<std::string>(*it);
         }
      friend void iter_swap(iterator const a, iterator const b)
         noexcept(noexcept(std::string(static_cast<std::string>(*a))) && noexcept(std::string(static_cast<std::string>(*b))))
         {
         if(a.aLines==b.aLines && a.iIndex==b.iIndex) return;
         std::string tmp = static_cast<std::string>(*a);
         *a = static_cast<std::string>(*b);
         *b = std::move(tmp);
         }
   private:
      System::Classes::TStrings* aLines{};
      int iIndex{};
      mutable LineRef theRef{};
   };

   explicit LinesView(owner_ty* const pOwner) : aLines{ ValidateAndGetSequence(pOwner) } {}

   size_type Size()  const { return (size_type)aLines->Count; }
   bool      Empty() const { return Size()==0U; }

   iterator begin() const { return iterator{ aLines, 0 }; }
   iterator end()   const { return iterator{ aLines, (int)Size() }; }

   LineRef operator[](size_type i) const {
      if(i>=Size()) throw std::out_of_range("LinesView::operator[] index out of range");
      return LineRef{ aLines, (int)i };
   }

   void PushBack(std::string const& s) const { aLines->Add(s.c_str()); }

   template <std::ranges::input_range R>
      requires std::convertible_to<std::ranges::range_value_t<R>, std::string>
   void ReplaceAll(R const& r) const {
      aLines->BeginUpdate();
      try {
         aLines->Clear();
         for (std::string const& line : r) aLines->Add(line.c_str());
      } catch(...) { aLines->EndUpdate(); throw; }
      aLines->EndUpdate();
   }

   template <std::ranges::forward_range R>
      requires std::convertible_to<std::ranges::range_value_t<R>, std::string>
   void UpdateInPlace(R const& r) const {
      std::vector<std::string> vec(std::ranges::begin(r), std::ranges::end(r));
      std::size_t const oldN = (std::size_t)aLines->Count;
      std::size_t const newN = vec.size();

      std::size_t common = 0;
      for(std::size_t u=0; u<std::min(oldN,newN); ++u) {
         if(std::string(aLines->Strings[(int)u].c_str()) != vec[u]) break;
         ++common;
      }

      aLines->BeginUpdate();
      try {
         for(std::size_t u=common; u<newN && u<oldN; ++u)
            aLines->Strings[(int)u] = vec[u].c_str();
         for(std::size_t u=oldN; u<newN; ++u)
            aLines->Add(vec[u].c_str());
         for(std::size_t u=newN; u<oldN; ++u)
            aLines->Delete((int)(aLines->Count-1));
      } catch(...) { aLines->EndUpdate(); throw; }
      aLines->EndUpdate();
   }

   template <std::ranges::input_range R>
      requires std::convertible_to<std::ranges::range_value_t<R>, std::string>
   void AppendRange(R const& r) const {
      aLines->BeginUpdate();
      try {
         for (std::string const& s : r) aLines->Add(s.c_str());
      } catch(...) { aLines->EndUpdate(); throw; }
      aLines->EndUpdate();
   }

   template <std::ranges::input_range R>
      requires std::convertible_to<std::ranges::range_value_t<R>, std::string>
   void InsertAt(size_type pos, R const& r) const {
      if(pos>Size()) throw std::out_of_range("LinesView::InsertAt out of range");
      aLines->BeginUpdate();
      try {
         int i = (int)pos;
         for (std::string const& s : r) aLines->Insert(i++, s.c_str());
      } catch(...) { aLines->EndUpdate(); throw; }
      aLines->EndUpdate();
   }

   void Erase(size_type pos) const {
      if(pos>=Size()) throw std::out_of_range("LinesView::Erase out of range");
      aLines->Delete((int)pos);
   }

   void EraseRange(size_type first, size_type last) const {
      if(first>last || last>Size()) throw std::out_of_range("LinesView::EraseRange invalid");
      aLines->BeginUpdate();
      try {
         for(size_type u=first; u<last; ++u) aLines->Delete((int)first);
      } catch(...) { aLines->EndUpdate(); throw; }
      aLines->EndUpdate();
   }

   System::Classes::TStrings* Underlying() const { return aLines; }

private:
   static System::Classes::TStrings* ValidateAndGetSequence(owner_ty* const pOwner) {
      if(!pOwner) throw std::invalid_argument("LinesView: owner null");

      if constexpr (HasLinesProp<owner_ty>) {
         if(!pOwner->Lines) throw std::invalid_argument("LinesView: Lines null");
         return pOwner->Lines;
         } 
      else if constexpr (HasItemsProp<owner_ty>) {
         if(!pOwner->Items) throw std::invalid_argument("LinesView: Items null");
         return pOwner->Items; // bereits TStrings*
         } 
      else {
         static_assert(always_false_v<owner_ty>, "Owner has neither Lines nor Items");
         }
      }

   System::Classes::TStrings* aLines{};
};

// Deduction Guide + Helper
template <HasLinesOrItems owner_ty>
LinesView(owner_ty*) -> LinesView<owner_ty>;

template <HasLinesOrItems owner_ty>
LinesView<owner_ty> MakeLinesView(owner_ty* const pOwner) {
   return LinesView<owner_ty>{pOwner};
}


// -------------------------------------------------------------
// Output-Iterator: hängt jede Zuweisung als NEUE ZEILE an
// -------------------------------------------------------------
template <HasLinesOrItems owner_ty>
class LinesBackInserter {
public:
   // Anforderungen für Ranges-Algorithmen:
   using iterator_concept  = std::output_iterator_tag;
   using iterator_category = std::output_iterator_tag;
   using difference_type   = std::ptrdiff_t;   // <- WICHTIG: signed
   using value_type        = void;
   using reference         = void;
   using pointer           = void;

   LinesBackInserter() = default;
   LinesBackInserter(LinesBackInserter const&)            = default;
   LinesBackInserter(LinesBackInserter&&)                 = default;
   LinesBackInserter& operator=(LinesBackInserter const&) = default;
   LinesBackInserter& operator=(LinesBackInserter&&)      = default;

   // Robuste Ctors:
   explicit LinesBackInserter(System::Classes::TStrings* p) : pLines_{p} {}

   explicit LinesBackInserter(LinesView<owner_ty>& view)            : pLines_{view.Underlying()} {}
   explicit LinesBackInserter(LinesView<owner_ty> const& view)      : pLines_{view.Underlying()} {}
   explicit LinesBackInserter(LinesView<owner_ty>&& view)           : pLines_{view.Underlying()} {} // falls jemand std::move(...) übergibt

   // Zuweisung eines std::string
   LinesBackInserter& operator=(std::string const& s) {
      pLines_->Add(s.c_str());
      return *this;
      }


   // Zuweisung beliebiger in std::string konvertierbarer Werte
   template <typename ty>
   requires requires (ty const& v) { ConvertTo<System::UnicodeString>(v); }
   LinesBackInserter& operator=(ty const& v) {
      pLines_->Add(ConvertTo<System::UnicodeString>(v));
      return *this;
      }

   // Output-Iterator Interface
   LinesBackInserter& operator*()     { return *this; }
   LinesBackInserter& operator++()    { return *this; }
   LinesBackInserter& operator++(int) { return *this; }

private:
   System::Classes::TStrings* pLines_{};  // nicht-owning
};

// Bequeme Fabriken – vermeiden Referenzprobleme
template <HasLinesOrItems owner_ty>
inline LinesBackInserter<owner_ty> make_lines_back_inserter(LinesView<owner_ty>& view) {
   return LinesBackInserter<owner_ty>{ view.Underlying() };
   }
   
template <HasLinesOrItems owner_ty>
inline LinesBackInserter<owner_ty> make_lines_back_inserter(LinesView<owner_ty> const& view) {
   return LinesBackInserter<owner_ty>{ view.Underlying() };
   }
   
template <HasLinesOrItems owner_ty>
inline LinesBackInserter<owner_ty> make_lines_back_inserter(owner_ty* owner) {
   LinesView<owner_ty> lv{ owner };             // temporär ok
   return LinesBackInserter<owner_ty>{ lv.Underlying() }; // roher Pointer bleibt gültig
   }

} // end of namespace adecc::delphi



/*
template <HasLinesOrItems OwnerT>
class LinesView {
public:
   using difference_type = std::ptrdiff_t;
   using size_type = std::size_t;

class LineRef {
public:
   // NEU: für Iterator-Pufferung
   LineRef() = default;
   LineRef(System::Classes::TStrings *const aLines, int const iIndex)
	  : aLines{aLines}, iIndex{iIndex} { }

   // NEU: reguläre Special Member Functions explizit erlauben
   LineRef(LineRef const&)            = default;
   LineRef(LineRef&&)                 = default;
   LineRef & operator=(LineRef const&) = default;
   LineRef & operator=(LineRef&&)      = default;

   operator std::string() const {
	  return std::string(AnsiString(aLines->Strings[iIndex]).c_str());
   }

   LineRef & operator=(std::string const& strValue) {
      aLines->Strings[iIndex] = strValue.c_str();
      return *this;
   }

   LineRef & operator=(char const * const szCStr) {
	  aLines->Strings[iIndex] = szCStr;
      return *this;
   }

   LineRef & operator+=(std::string const & strSuffix) {
      System::UnicodeString theUni = aLines->Strings[iIndex];
	  theUni += strSuffix.c_str();
      aLines->Strings[iIndex] = theUni;
      return *this;
   }

private:
   System::Classes::TStrings *aLines{};
   int iIndex{};
};

class iterator {
public:
   using iterator_concept  = std::random_access_iterator_tag;
   using iterator_category = std::random_access_iterator_tag;
   using value_type        = std::string;
   using difference_type   = std::ptrdiff_t;
   using reference         = LineRef; // Traits-Info; maßgeblich ist decltype(*it)

   iterator() = default;
   iterator(System::Classes::TStrings *const aLines, int const iIndex) : aLines{aLines}, iIndex{iIndex} { }

   // lvalue-Proxy: Referenz auf intern gepufferten LineRef
   LineRef & operator * () const {
      theRef = LineRef{aLines, iIndex};
	  return theRef;
   }

   // Konsistent zu *(this + n), ebenfalls lvalue-Proxy
   LineRef & operator [] (difference_type const iN) const {
      theRef = LineRef{aLines, static_cast<int>(iIndex + iN)};
	  return theRef;
   }

   iterator & operator++() { ++iIndex; return *this; }
   iterator operator++(int) { iterator tmp=*this; ++(*this); return tmp; }
   iterator & operator--() { --iIndex; return *this; }
   iterator operator --(int) { iterator tmp=*this; --(*this); return tmp; }

   iterator & operator += (difference_type const iN) { iIndex += static_cast<int>(iN); return *this; }
   iterator & operator -= (difference_type const iN) { iIndex -= static_cast<int>(iN); return *this; }
   friend iterator operator + (iterator it, difference_type const iN) { it += iN; return it; }
   friend iterator operator + (difference_type const iN, iterator it) { it += iN; return it; }
   friend iterator operator - (iterator it, difference_type const iN) { it -= iN; return it; }

   friend difference_type operator - (iterator const & a, iterator const & b) {
	  return static_cast<difference_type>(a.iIndex) - static_cast<difference_type>(b.iIndex);
   }

   friend bool operator == (iterator const & a, iterator const & b) {
      return a.aLines == b.aLines && a.iIndex == b.iIndex;
   }

   friend std::strong_ordering operator <=> (iterator const & a, iterator const & b) {
      if(a.aLines != b.aLines) return std::compare_three_way()(a.aLines, b.aLines);
	  return a.iIndex <=> b.iIndex;
   }

   // --------- Proxy-freundliche Hooks ---------
   friend std::string iter_move(iterator const it) noexcept {
      return static_cast<std::string>(*it);
   }

   friend void iter_swap(iterator const a, iterator const b)
	  noexcept(noexcept(std::string(static_cast<std::string>(*a))) &&
               noexcept(std::string(static_cast<std::string>(*b))))
   {
	  if(a.aLines == b.aLines && a.iIndex == b.iIndex) return;
      std::string theTmp = static_cast<std::string>(*a);
      *a = static_cast<std::string>(*b);
	  *b = std::move(theTmp);
   }

private:
   System::Classes::TStrings *aLines{};
   int iIndex{};

   // Puffer für lvalue-Proxy; muss veränderbar in const-Kontext sein
   mutable LineRef theRef{};
};


   explicit LinesView(OwnerT *const pOwner) : aLines{ValidateAndGetLines(pOwner)} { }

   size_type Size() const { return static_cast<size_type>(aLines->Count); }
   bool Empty() const { return Size()==0U; }

   iterator begin() const { return iterator{aLines,0}; }
   iterator end() const { return iterator{aLines, static_cast<int>(Size())}; }

   LineRef operator[](size_type const i) const {
	  if(i>=Size()) throw std::out_of_range("LinesView::operator[] index out of range");
	  return LineRef{aLines, static_cast<int>(i)};
	  }

   void PushBack(std::string const & strValue) const {
	  aLines->Add(strValue.c_str());
	  }

   template <std::ranges::input_range RangeT>
		requires std::convertible_to<std::ranges::range_value_t<RangeT>, std::string>
   void ReplaceAll(RangeT const & theRange) const {
	  aLines->BeginUpdate();
	  try {
		 aLines->Clear();
		 for(std::string const & strLine : theRange) {
			aLines->Add(strLine.c_str());
			}
		 }
	  catch(...) { aLines->EndUpdate(); throw; }
	  aLines->EndUpdate();
	  }

   template <std::ranges::forward_range RangeT>
		requires std::convertible_to<std::ranges::range_value_t<RangeT>, std::string>
   void UpdateInPlace(RangeT const & theRange) const {
	  std::vector<std::string> vecNew(std::ranges::begin(theRange), std::ranges::end(theRange));
	  std::size_t const uOld = static_cast<std::size_t>(aLines->Count);
	  std::size_t const uNew = vecNew.size();

	  std::size_t uCommon = 0U;
	  for(std::size_t u = 0; u < std::min(uOld, uNew); ++u) {
		 if(std::string(aLines->Strings[static_cast<int>(u)].c_str())!=vecNew[u]) break;
		 ++uCommon;
		 }

	  aLines->BeginUpdate();
	  try {
		 for(std::size_t u=uCommon; u<uNew && u<uOld; ++u) {
			aLines->Strings[static_cast<int>(u)] = vecNew[u].c_str();
			}

		 for(std::size_t u=uOld; u<uNew; ++u) {
			aLines->Add(vecNew[u].c_str());
			}

		 for(std::size_t u=uNew; u<uOld; ++u) {
			aLines->Delete(static_cast<int>(aLines->Count-1));
			}
		 }
	  catch(...) { aLines->EndUpdate(); throw; }
	  aLines->EndUpdate();
	  }

   template <std::ranges::input_range RangeT>
		  requires std::convertible_to<std::ranges::range_value_t<RangeT>, std::string>
   void AppendRange(RangeT const & theRange) const {
	  aLines->BeginUpdate();
	  try {
		 for(std::string const & s : theRange) {
			aLines->Add(s.c_str());
			}
		 }
	  catch(...) { aLines->EndUpdate(); throw; }
	  aLines->EndUpdate();
   }

   template <std::ranges::input_range RangeT>
		   requires std::convertible_to<std::ranges::range_value_t<RangeT>, std::string>
   void InsertAt(size_type const iPos, RangeT const & theRange) const {
	  if(iPos>Size()) throw std::out_of_range("LinesView::InsertAt out of range");
	  aLines->BeginUpdate();
	  try {
		 int i = static_cast<int>(iPos);
		 for(std::string const & s : theRange) {
			aLines->Insert(i++, s.c_str());
			}
		 }
	  catch(...) { aLines->EndUpdate(); throw; }
	  aLines->EndUpdate();
	  }

   void Erase(size_type const iPos) const {
	  if(iPos>=Size()) throw std::out_of_range("LinesView::Erase out of range");
	  aLines->Delete(static_cast<int>(iPos));
	  }

   void EraseRange(size_type const iFirst, size_type const iLast) const {
	  if(iFirst>iLast || iLast>Size()) throw std::out_of_range("LinesView::EraseRange invalid");
	  aLines->BeginUpdate();
	  try {
		 for(size_type u=iFirst; u<iLast; ++u) {
			aLines->Delete(static_cast<int>(iFirst));
			}
		 }
	  catch(...) { aLines->EndUpdate(); throw; }
	  aLines->EndUpdate();
	  }

   System::Classes::TStrings * Underlying() const { return aLines; }

private:
   static System::Classes::TStrings * ValidateAndGetLines(OwnerT *const pOwner) {
	  if(!pOwner)        throw std::invalid_argument("LinesView: owner null");
	  if(!pOwner->Lines) throw std::invalid_argument("LinesView: Lines null");
	  return pOwner->Lines;
	  }

   System::Classes::TStrings *aLines{};
};

// Deduction Guide
template <HasLinesMember OwnerT>
LinesView(OwnerT *) -> LinesView<OwnerT>;

template <HasLinesMember OwnerT>
LinesView<OwnerT> MakeLinesView(OwnerT *const pOwner) {
   return LinesView<OwnerT>{pOwner};
   }

*/
