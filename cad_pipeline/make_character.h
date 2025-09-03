#pragma once

#include <optional>
#include <print>

#include "boost/geometry.hpp"
#include "boost/geometry/geometries/geometries.hpp"
#include "ttf2mesh.h"
#include "types.h"

// Glyph functions adapted from ttf2mesh example
std::optional<ttf_t *> LoadSystemFont() {
  // list all system fonts by filename mask:

  ttf_t *font = nullptr;

  ttf_t **list =
      ttf_list_system_fonts("DejaVuSans*|Ubuntu*|FreeSerif*|Arial*|Cour*");
  if (list == NULL) return {};     // no memory in system
  if (list[0] == NULL) return {};  // no fonts were found

  // load the first font from the list

  ttf_load_from_file(list[0]->filename, &font, false);
  ttf_free_list(list);
  return font;
}

std::optional<ttf_outline *> GetGlyphOutline(ttf_t *font, char symbol) {
  ttf_glyph_t *glyph = nullptr;

  // find a glyph in the font file

  int index = ttf_find_glyph(font, symbol);
  if (index < 0) {
    return {};
  }

  glyph = &font->glyphs[index];

  auto outline = ttf_linear_outline(glyph, TTF_QUALITY_NORMAL);
  if (!outline) {
    return {};
  }
  return outline;
}

internal::polygon_t ConvertToPolygon(ttf_outline *outline) {
  internal::polygon_t poly;

  for (auto ci = 0; ci < outline->ncontours; ++ci) {
    internal::polygon_t test_poly;

    for (auto i = 0; i < outline->cont[ci].length; i++) {
      const auto &pt = outline->cont[ci].pt[i];
      boost::geometry::append(test_poly.outer(),
                              internal::point_t(5.f * pt.x, 5.f * pt.y));
    }
    if (poly.outer().empty()) {
      poly.outer() = test_poly.outer();
    } else {
      if (boost::geometry::within(test_poly, poly.outer())) {
        poly.inners().push_back(test_poly.outer());
        
      } else {
        std::swap(poly.outer(), test_poly.outer());
        poly.inners().push_back(test_poly.outer());
      }
    }
  }

  return poly;
}

AnyGeometry MakeCharacterPolygon(char symbol) {
  std::println("MakeCharacterPolygon({})", symbol);
  std::flush(std::cout);
  auto font = LoadSystemFont();
  if (!font) {
    std::println("Could not load font");
    return {};
  }

  auto outline = GetGlyphOutline(font.value(), symbol);
  if (!outline) {
    std::println("Could not get glyph outline");
    return {};
  }

  const auto &polygon = ConvertToPolygon(outline.value());

  ttf_free_outline(outline.value());
  ttf_free(font.value());

  return make_shared<Polygon>(polygon);
}