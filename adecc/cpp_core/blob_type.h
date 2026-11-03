// SPDX-FileCopyrightText: 2021 - 2026 adecc Systemhaus GmbH
// SPDX-License-Identifier: LicenseRef-PolyForm-Noncommercial-1.0.0

/**
\file blob_type.h
\brief Blob Type as generic Value or Parameter for database operations.

\version 1.0
\date 03.11.2026
\author Volker Hillmann (adecc Systemhaus GmbH)

\copyright Copyright © 2021 - 2026 adecc Systemhaus GmbH

\licenseblock{LicenseRef-PolyForm-Noncommercial-1.0.0}
This file is licensed under the PolyForm Noncommercial License 1.0.0.
Use, modification, and distribution are permitted only as defined by that license.
The complete and controlling terms are available at
https://polyformproject.org/licenses/noncommercial/1.0.0/.
Any use not permitted by that license requires separate permission or a separate
license from adecc Systemhaus GmbH.
\endlicenseblock

*/

#pragma once

#include <cstddef>
#include <span>
#include <utility>
#include <vector>

namespace adecc {
   /**
   \brief Owns a binary data sequence.
   \details
      Provides owning storage for arbitrary binary data and
      non-owning views for database bindings.

   \tparam None.
   \param None.
   \returns None.
   \throw std::bad_alloc If memory allocation fails.
   \note Views are invalidated when the underlying storage changes.
   \pre None.
   \post The owned binary data remains available for the lifetime
      of the object.
   */
   struct Blob {
      std::vector<std::byte> vecData;

      Blob() = default;
      explicit Blob(std::vector<std::byte> vecBytes) : vecData { std::move(vecBytes) } { }

      [[nodiscard]] std::span<std::byte const> View() const noexcept { return vecData; }
      [[nodiscard]] std::span<std::byte> View() noexcept { return vecData; }

      [[nodiscard]] std::size_t Size() const noexcept { return vecData.size(); }
   };
} // end of namespace adecc
