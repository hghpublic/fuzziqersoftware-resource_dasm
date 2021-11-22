#include "QuickDrawEngine.hh"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <exception>
#include <phosg/Encoding.hh>
#include <phosg/Image.hh>
#include <phosg/Strings.hh>
#include <phosg/Time.hh>
#include <phosg/Filesystem.hh>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <string>

#include "QuickDrawFormats.hh"

using namespace std;



pict_contains_undecodable_quicktime::pict_contains_undecodable_quicktime(
    string&& ext, string&& data)
  : extension(move(ext)), data(move(data)) { }



QuickDrawPortInterface::~QuickDrawPortInterface() { }



struct PictColorTable {
  string data;
  ColorTable* table;

  PictColorTable(StringReader& r) {
    size_t size = r.get<ColorTable>(false).size_swapped();
    this->data = r.read(size);
    this->table = reinterpret_cast<ColorTable*>(const_cast<char*>(this->data.data()));
    this->table->byteswap();
  }
};



struct PictSubheaderV2 {
  int32_t version; // == -1
  Fixed bounds_x1;
  Fixed bounds_y1;
  Fixed bounds_x2;
  Fixed bounds_y2;
  uint32_t reserved2;

  void byteswap() {
    this->version = bswap16(this->version);
    this->bounds_y1.byteswap();
    this->bounds_x1.byteswap();
    this->bounds_y2.byteswap();
    this->bounds_x2.byteswap();
  }
} __attribute__ ((packed));

struct PictSubheaderV2Extended {
  int16_t version; // == -2
  uint16_t reserved1;
  Fixed horizontal_resolution_dpi;
  Fixed vertical_resolution_dpi;
  Rect source_rect;
  uint16_t reserved2;

  void byteswap() {
    this->version = bswap16(this->version);
    this->horizontal_resolution_dpi.byteswap();
    this->vertical_resolution_dpi.byteswap();
    this->source_rect.byteswap();
  }
} __attribute__ ((packed));

union PictSubheader {
  PictSubheaderV2 v2;
  PictSubheaderV2Extended v2e;
};



void QuickDrawEngine::set_port(QuickDrawPortInterface* port) {
  this->port = port;
}



void QuickDrawEngine::write_canvas_pixel(ssize_t x, ssize_t y, uint64_t r,
    uint64_t g, uint64_t b, uint64_t a) {
  if (!this->port->get_clip_region().contains(x, y) || !this->port->get_bounds().contains(x, y)) {
    return;
  }
  this->port->write_pixel(x - this->pict_bounds.x1, y - this->pict_bounds.y1, r, g, b);
}

pair<Pattern, Image> QuickDrawEngine::pict_read_pixel_pattern(StringReader& r) {
  uint16_t type = r.get_u16r();
  Pattern monochrome_pattern = r.get<Pattern>();

  if (type == 1) { // normal pattern
    PixelMapHeader header = r.get<PixelMapHeader>();
    header.byteswap();
    PictColorTable ctable(r);

    uint16_t row_bytes = header.flags_row_bytes & 0x7FFF;
    string data = r.read(header.bounds.height() * row_bytes);
    const PixelMapData* pixel_map = reinterpret_cast<const PixelMapData*>(data.data());

    return make_pair(monochrome_pattern, decode_color_image(header, *pixel_map, ctable.table));

  } else if (type == 2) { // dither pattern
    Color c = r.get<Color>();
    c.byteswap();
    // TODO: figure out how dither patterns work
    throw runtime_error("dither patterns are not supported");

  } else {
    throw runtime_error("unknown pattern type");
  }
}

void QuickDrawEngine::pict_skip_0(StringReader& r, uint16_t opcode) { }

void QuickDrawEngine::pict_skip_2(StringReader& r, uint16_t opcode) {
  r.go(r.where() + 2);
}
void QuickDrawEngine::pict_skip_8(StringReader& r, uint16_t opcode) {
  r.go(r.where() + 8);
}

void QuickDrawEngine::pict_skip_12(StringReader& r, uint16_t opcode) {
  r.go(r.where() + 12);
}

void QuickDrawEngine::pict_skip_var16(StringReader& r, uint16_t opcode) {
  uint16_t len = r.get_u16r();
  r.go(r.where() + len);
}

void QuickDrawEngine::pict_skip_var32(StringReader& r, uint16_t opcode) {
  uint32_t len = r.get_u32r();
  r.go(r.where() + len);
}

void QuickDrawEngine::pict_skip_long_comment(StringReader& r, uint16_t opcode) {
  r.go(r.where() + 2); // type (unused)
  uint16_t size = r.get_u16r();
  r.go(r.where() + size);
}

void QuickDrawEngine::pict_unimplemented_opcode(StringReader& r, uint16_t opcode) {
  throw runtime_error(string_printf("unimplemented opcode %04hX before offset %zX",
      opcode, r.where()));
}



// state modification opcodes

void QuickDrawEngine::pict_set_clipping_region(StringReader& r, uint16_t opcode) {
  Region rgn(r);
  this->port->set_clip_region(move(rgn));
}

void QuickDrawEngine::pict_set_font_number(StringReader& r, uint16_t opcode) {
  this->port->set_text_font(r.get_u16r());
}

void QuickDrawEngine::pict_set_font_style_flags(StringReader& r, uint16_t opcode) {
  this->port->set_text_style(r.get_u8());
}

void QuickDrawEngine::pict_set_text_source_mode(StringReader& r, uint16_t opcode) {
  this->port->set_text_mode(r.get_u16r());
}

void QuickDrawEngine::pict_set_text_extra_space(StringReader& r, uint16_t opcode) {
  Fixed x = r.get<Fixed>();
  x.byteswap();
  this->port->set_extra_space_space(x);
}

void QuickDrawEngine::pict_set_text_nonspace_extra_width(StringReader& r, uint16_t opcode) {
  this->port->set_extra_space_nonspace(r.get_u16r());
}

void QuickDrawEngine::pict_set_font_number_and_name(StringReader& r, uint16_t opcode) {
  uint16_t data_size = r.get_u16r();
  this->port->set_text_font(r.get_u16r());
  uint8_t font_name_bytes = r.get_u8();
  if (font_name_bytes != data_size - 3) {
    throw runtime_error("font name length does not align with command data length");
  }
  // TODO: should we do anything with the font name?
}

void QuickDrawEngine::pict_set_pen_size(StringReader& r, uint16_t opcode) {
  Point p = r.get<Point>();
  p.byteswap();
  this->port->set_pen_size(p);
}

void QuickDrawEngine::pict_set_pen_mode(StringReader& r, uint16_t opcode) {
  this->port->set_pen_mode(r.get_u16r());
}

void QuickDrawEngine::pict_set_background_pattern(StringReader& r, uint16_t opcode) {
  Pattern p = r.get<Pattern>();
  this->port->set_background_mono_pattern(p);
  this->port->set_background_pixel_pattern(Image(0, 0));
}

void QuickDrawEngine::pict_set_pen_pattern(StringReader& r, uint16_t opcode) {
  Pattern p = r.get<Pattern>();
  this->port->set_pen_mono_pattern(p);
  this->port->set_pen_pixel_pattern(Image(0, 0));
}

void QuickDrawEngine::pict_set_fill_pattern(StringReader& r, uint16_t opcode) {
  Pattern p = r.get<Pattern>();
  this->port->set_fill_mono_pattern(p);
  this->port->set_fill_pixel_pattern(Image(0, 0));
}

void QuickDrawEngine::pict_set_background_pixel_pattern(StringReader& r, uint16_t opcode) {
  auto p = this->pict_read_pixel_pattern(r);
  this->port->set_background_mono_pattern(p.first);
  this->port->set_background_pixel_pattern(move(p.second));
}

void QuickDrawEngine::pict_set_pen_pixel_pattern(StringReader& r, uint16_t opcode) {
  auto p = this->pict_read_pixel_pattern(r);
  this->port->set_pen_mono_pattern(p.first);
  this->port->set_pen_pixel_pattern(move(p.second));
}

void QuickDrawEngine::pict_set_fill_pixel_pattern(StringReader& r, uint16_t opcode) {
  auto p = this->pict_read_pixel_pattern(r);
  this->port->set_fill_mono_pattern(p.first);
  this->port->set_fill_pixel_pattern(move(p.second));
}

void QuickDrawEngine::pict_set_oval_size(StringReader& r, uint16_t opcode) {
  this->pict_oval_size = r.get<Point>();
  this->pict_oval_size.byteswap();
}

void QuickDrawEngine::pict_set_origin_dh_dv(StringReader& r, uint16_t opcode) {
  this->pict_origin = r.get<Point>();
  this->pict_origin.byteswap();
}

void QuickDrawEngine::pict_set_text_ratio(StringReader& r, uint16_t opcode) {
  this->pict_text_ratio_numerator = r.get<Point>();
  this->pict_text_ratio_numerator.byteswap();
  this->pict_text_ratio_denominator = r.get<Point>();
  this->pict_text_ratio_denominator.byteswap();
}

void QuickDrawEngine::pict_set_text_size(StringReader& r, uint16_t opcode) {
  this->port->set_text_size(r.get_u16r());
}

void QuickDrawEngine::pict_set_foreground_color32(StringReader& r, uint16_t opcode) {
  uint32_t color = r.get_u32r();
  Color c(
      ((color >> 8) & 0xFF00) | ((color >> 16) & 0x00FF),
      ((color >> 0) & 0xFF00) | ((color >> 8) & 0x00FF),
      ((color << 8) & 0xFF00) | ((color >> 0) & 0x00FF));
  this->port->set_foreground_color(c);
}

void QuickDrawEngine::pict_set_background_color32(StringReader& r, uint16_t opcode) {
  uint32_t color = r.get_u32r();
  Color c(
      ((color >> 8) & 0xFF00) | ((color >> 16) & 0x00FF),
      ((color >> 0) & 0xFF00) | ((color >> 8) & 0x00FF),
      ((color << 8) & 0xFF00) | ((color >> 0) & 0x00FF));
  this->port->set_background_color(c);
}

void QuickDrawEngine::pict_set_version(StringReader& r, uint16_t opcode) {
  this->pict_version = r.get_u8();
  if (this->pict_version != 1 && this->pict_version != 2) {
    throw runtime_error("version is not 1 or 2");
  }
  if ((this->pict_version == 2) && (r.get_u8() != 0xFF)) {
    throw runtime_error("version 2 picture is not version 02FF");
  }
}

void QuickDrawEngine::pict_set_highlight_mode_flag(StringReader& r, uint16_t opcode) {
  this->pict_highlight_flag = true;
}

void QuickDrawEngine::pict_set_highlight_color(StringReader& r, uint16_t opcode) {
  Color c = r.get<Color>();
  c.byteswap();
  this->port->set_highlight_color(c);
}

void QuickDrawEngine::pict_set_foreground_color(StringReader& r, uint16_t opcode) {
  Color c = r.get<Color>();
  c.byteswap();
  this->port->set_foreground_color(c);
}

void QuickDrawEngine::pict_set_background_color(StringReader& r, uint16_t opcode) {
  Color c = r.get<Color>();
  c.byteswap();
  this->port->set_background_color(c);
}

void QuickDrawEngine::pict_set_op_color(StringReader& r, uint16_t opcode) {
  Color c = r.get<Color>();
  c.byteswap();
  this->port->set_op_color(c);
}

void QuickDrawEngine::pict_set_default_highlight_color(StringReader& r, uint16_t opcode) {
  this->port->set_highlight_color(this->default_highlight_color);
}



// simple shape opcodes

void QuickDrawEngine::pict_fill_current_rect_with_pattern(const Pattern& pat, const Image& pixel_pat) {
  if (pixel_pat.get_width() && pixel_pat.get_height()) {
    for (ssize_t y = this->pict_last_rect.y1; y < this->pict_last_rect.y2; y++) {
      for (ssize_t x = this->pict_last_rect.x1; x < this->pict_last_rect.x2; x++) {
        uint64_t r, g, b;
        pixel_pat.read_pixel(x % pixel_pat.get_width(), y % pixel_pat.get_height(), &r, &g, &b);
        this->write_canvas_pixel(x, y, r, g, b);
      }
    }
  } else {
    for (ssize_t y = this->pict_last_rect.y1; y < this->pict_last_rect.y2; y++) {
      for (ssize_t x = this->pict_last_rect.x1; x < this->pict_last_rect.x2; x++) {
        uint8_t value = pat.pixel_at(x - this->pict_bounds.x1, y - this->pict_bounds.y1) ? 0x00 : 0xFF;
        this->write_canvas_pixel(x, y, value, value, value);
      }
    }
  }
}

void QuickDrawEngine::pict_erase_last_rect(StringReader& r, uint16_t opcode) {
  this->pict_fill_current_rect_with_pattern(this->port->get_background_mono_pattern(),
      this->port->get_background_pixel_pattern());
}

void QuickDrawEngine::pict_erase_rect(StringReader& r, uint16_t opcode) {
  this->pict_last_rect = r.get<Rect>();
  this->pict_last_rect.byteswap();
  this->pict_fill_current_rect_with_pattern(this->port->get_background_mono_pattern(),
      this->port->get_background_pixel_pattern());
}

void QuickDrawEngine::pict_fill_last_rect(StringReader& r, uint16_t opcode) {
  this->pict_fill_current_rect_with_pattern(this->port->get_fill_mono_pattern(),
      this->port->get_fill_pixel_pattern());
}

void QuickDrawEngine::pict_fill_rect(StringReader& r, uint16_t opcode) {
  this->pict_last_rect = r.get<Rect>();
  this->pict_last_rect.byteswap();
  this->pict_fill_last_rect(r, opcode);
}

void QuickDrawEngine::pict_fill_last_oval(StringReader& r, uint16_t opcode) {
  double x_center = static_cast<double>(this->pict_last_rect.x2 + this->pict_last_rect.x1) / 2.0;
  double y_center = static_cast<double>(this->pict_last_rect.y2 + this->pict_last_rect.y1) / 2.0;
  double width = this->pict_last_rect.x2 - this->pict_last_rect.x1;
  double height = this->pict_last_rect.y2 - this->pict_last_rect.y1;
  auto fill_pat = this->port->get_fill_mono_pattern();
  for (ssize_t y = this->pict_last_rect.y1; y < this->pict_last_rect.y2; y++) {
    for (ssize_t x = this->pict_last_rect.x1; x < this->pict_last_rect.x2; x++) {
      double x_dist = (static_cast<double>(x) - x_center) / width;
      double y_dist = (static_cast<double>(y) - y_center) / height;
      if (x_dist * x_dist + y_dist * y_dist > 0.25) {
        continue;
      }
      uint8_t value = fill_pat.pixel_at(x - this->pict_bounds.x1, y - this->pict_bounds.y1) ? 0x00 : 0xFF;
      this->write_canvas_pixel(x, y, value, value, value);
    }
  }
}

void QuickDrawEngine::pict_fill_oval(StringReader& r, uint16_t opcode) {
  this->pict_last_rect = r.get<Rect>();
  this->pict_last_rect.byteswap();
  this->pict_fill_last_oval(r, opcode);
}



// bits opcodes

struct PictCopyBitsMonochromeArgs {
  BitMapHeader header;
  Rect source_rect;
  Rect dest_rect;
  uint16_t mode;

  void byteswap() {
    this->header.byteswap();
    this->source_rect.byteswap();
    this->dest_rect.byteswap();
    this->mode = bswap16(this->mode);
  }
};

/* there's no struct PictPackedCopyBitsIndexedColorArgs because the color table
 * is a variable size and comes early in the format. if there were such a struct
 * it would look like this:
 * struct PictPackedCopyBitsIndexedColorArgs {
 *   PixelMapHeader header;
 *   ColorTable ctable; // variable size
 *   rect source_rect;
 *   rect dest_rect;
 *   uint16_t mode;
 * };
 */

string QuickDrawEngine::unpack_bits(StringReader& r, size_t w, size_t h,
    uint16_t row_bytes, bool sizes_are_words, bool chunks_are_words) {
  string ret;
  size_t expected_size = row_bytes * h;
  ret.reserve(expected_size);

  for (size_t y = 0; y < h; y++) {
    uint16_t packed_row_bytes = sizes_are_words ? r.get_u16r() : r.get_u8();
    for (size_t row_end_offset = r.where() + packed_row_bytes; r.where() < row_end_offset;) {
      int16_t count = r.get_s8();
      if (count < 0) { // RLE segment
        if (chunks_are_words) {
          uint16_t value = r.get_u16r();
          for (ssize_t x = 0; x < -(count - 1); x++) {
            ret.push_back((value >> 8) & 0xFF);
            ret.push_back(value & 0xFF);
          }
        } else {
          ret.insert(ret.size(), -(count - 1), r.get_u8());
        }
      } else { // direct segment
        if (chunks_are_words) {
          ret += r.read((count + 1) * 2);
        } else {
          ret += r.read(count + 1);
        }
      }
    }
    if (ret.size() != static_cast<size_t>(row_bytes * (y + 1))) {
      throw runtime_error(string_printf("packed data size is incorrect on row %zu at offset %zX (expected %zX, have %zX)",
          y, r.where(), row_bytes * (y + 1), ret.size()));
    }
  }
  if (row_bytes * h != ret.size()) {
    throw runtime_error(string_printf("unpacked data size is incorrect (expected %zX, have %zX)",
        row_bytes * h, ret.size()));
  }
  return ret;
}

string QuickDrawEngine::unpack_bits(StringReader& r, size_t w, size_t h,
    uint16_t row_bytes, bool chunks_are_words) {
  size_t start_offset = r.where();
  string failure_strs[2];
  for (size_t x = 0; x < 2; x++) {
    try {
      // if row_bytes > 250, word sizes are most likely to be correct, so try
      // that first
      return unpack_bits(r, w, h, row_bytes, x ^ (row_bytes > 250), chunks_are_words);
    } catch (const exception& e) {
      failure_strs[x ^ (row_bytes > 250)] = e.what();
      r.go(start_offset);
    }
  }
  throw runtime_error(string_printf("failed to unpack data with either byte sizes (%s) or word sizes (%s)",
      failure_strs[0].c_str(), failure_strs[1].c_str()));
}

void QuickDrawEngine::pict_copy_bits_indexed_color(StringReader& r, uint16_t opcode) {
  bool is_packed = opcode & 0x08;
  bool has_mask_region = opcode & 0x01;

  Rect bounds;
  Rect source_rect;
  Rect dest_rect;
  Rect mask_region_rect;
  uint16_t mode __attribute__((unused));
  shared_ptr<Region> mask_region;
  Image source_image(0, 0);

  // TODO: should we support pixmaps in v1? currently we do, but I don't know if
  // this is technically correct behavior
  bool is_pixmap = r.get_u8(false) & 0x80;
  if (is_pixmap) {
    auto header = r.get<PixelMapHeader>();
    header.byteswap();
    bounds = header.bounds;

    PictColorTable ctable(r);

    source_rect = r.get<Rect>();
    source_rect.byteswap();
    dest_rect = r.get<Rect>();
    dest_rect.byteswap();
    // TODO: figure out where/how to use this
    /* uint16_t mode = */ r.get_u16r();

    if ((source_rect.width() != dest_rect.width()) ||
        (source_rect.height() != dest_rect.height())) {
      throw runtime_error("source and destination rect dimensions do not match");
    }

    if (has_mask_region) {
      mask_region.reset(new Region(r));
    }

    uint16_t row_bytes = header.flags_row_bytes & 0x7FFF;
    string data = is_packed ?
        unpack_bits(r, header.bounds.width(), header.bounds.height(), row_bytes, header.pixel_size == 0x10) :
        r.read(header.bounds.height() * row_bytes);
    const PixelMapData* pixel_map = reinterpret_cast<const PixelMapData*>(data.data());

    source_image = decode_color_image(header, *pixel_map, ctable.table);

  } else {
    auto args = r.get<PictCopyBitsMonochromeArgs>();
    args.byteswap();

    if (!args.header.bounds.contains(args.source_rect)) {
      string source_s = args.source_rect.str();
      string bounds_s = args.header.bounds.str();
      throw runtime_error(string_printf("source %s is not within bounds %s", source_s.c_str(), bounds_s.c_str()));
    }
    if ((args.source_rect.width() != args.dest_rect.width()) ||
        (args.source_rect.height() != args.dest_rect.height())) {
      throw runtime_error("source and destination rect dimensions do not match");
    }
    bounds = args.header.bounds;
    source_rect = args.source_rect;
    dest_rect = args.dest_rect;
    mode = args.mode;

    if (has_mask_region) {
      mask_region.reset(new Region(r));
    }

    string data = is_packed ?
        unpack_bits(r, args.header.bounds.width(), args.header.bounds.height(), args.header.flags_row_bytes, false) :
        r.read(args.header.bounds.height() * args.header.flags_row_bytes);
    source_image = decode_monochrome_image(data.data(), data.size(),
        args.header.bounds.width(), args.header.bounds.height(),
        args.header.flags_row_bytes);
  }

  // TODO: the clipping region should apply here
  if (mask_region.get() && !mask_region_rect.is_empty()) {
    if (mask_region_rect != source_rect) {
      throw runtime_error("mask region rect " + mask_region_rect.str() + " is not same as source rect " + source_rect.str());
    }
    this->port->blit(source_image,
        dest_rect.x1 - this->pict_bounds.x1,
        dest_rect.y1 - this->pict_bounds.y1,
        source_rect.x2 - source_rect.x1,
        source_rect.y2 - source_rect.y1,
        source_rect.x1 - bounds.x1,
        source_rect.y1 - bounds.y1,
        mask_region);
  } else {
    this->port->blit(source_image,
        dest_rect.x1 - this->pict_bounds.x1,
        dest_rect.y1 - this->pict_bounds.y1,
        source_rect.x2 - source_rect.x1,
        source_rect.y2 - source_rect.y1,
        source_rect.x1 - bounds.x1,
        source_rect.y1 - bounds.y1);
  }
}

struct PictPackedCopyBitsDirectColorArgs {
  uint32_t base_address; // unused
  PixelMapHeader header;
  Rect source_rect;
  Rect dest_rect;
  uint16_t mode;

  void byteswap() {
    this->header.byteswap();
    this->source_rect.byteswap();
    this->dest_rect.byteswap();
    this->mode = bswap16(this->mode);
  }
};

void QuickDrawEngine::pict_packed_copy_bits_direct_color(StringReader& r, uint16_t opcode) {
  bool has_mask_region = opcode & 0x01;

  auto args = r.get<PictPackedCopyBitsDirectColorArgs>();
  args.byteswap();

  if (!args.header.bounds.contains(args.source_rect)) {
    string source_s = args.source_rect.str();
    string bounds_s = args.header.bounds.str();
    throw runtime_error(string_printf("source %s is not within bounds %s", source_s.c_str(), bounds_s.c_str()));
  }
  if ((args.source_rect.width() != args.dest_rect.width()) ||
      (args.source_rect.height() != args.dest_rect.height())) {
    throw runtime_error("source and destination rect dimensions do not match");
  }

  shared_ptr<Image> mask_img;
  Rect mask_region_rect;
  if (has_mask_region) {
    Region mask_region(r);
    mask_region_rect = mask_region.rect;
    mask_img.reset(new Image(mask_region.render()));
  }

  size_t bytes_per_pixel;
  if (args.header.component_size == 8) {
    bytes_per_pixel = args.header.component_count;
    if ((args.header.component_count != 3) && (args.header.component_count != 4)) {
      throw runtime_error("for 5-bit channels, image must have 3 or 4 components");
    }
  } else if (args.header.component_size == 5) {
    // round up to the next byte boundary
    bytes_per_pixel = ((args.header.component_count * 5) + 7) / 8;
    if (args.header.component_count != 3) {
      throw runtime_error("for 5-bit channels, image must have 3 components");
    }
  } else {
    throw runtime_error("only 8-bit and 5-bit channels are supported");
  }
  size_t row_bytes = args.header.bounds.width() * bytes_per_pixel;
  string data = unpack_bits(r, args.header.bounds.width(), args.header.bounds.height(), row_bytes, args.header.pixel_size == 0x10);

  if (mask_img.get() && (mask_region_rect != args.source_rect)) {
    throw runtime_error("mask region rect is not same as source rect");
  }

  for (ssize_t y = 0; y < args.source_rect.height(); y++) {
    size_t row_offset = row_bytes * y;
    for (ssize_t x = 0; x < args.source_rect.width(); x++) {
      if (mask_img.get()) {
        uint64_t r, g, b;
        mask_img->read_pixel(x + args.source_rect.x1 - mask_region_rect.x1,
            y + args.source_rect.y1 - mask_region_rect.y1, &r, &g, &b);
        if (r || g || b) {
          continue;
        }
      }

      uint8_t r_value, g_value, b_value;
      if ((args.header.component_size == 8) && (args.header.component_count == 3)) {
        r_value = data[row_offset + x];
        g_value = data[row_offset + (row_bytes / 3) + x];
        b_value = data[row_offset + (2 * row_bytes / 3) + x];

      } else if ((args.header.component_size == 8) && (args.header.component_count == 4)) {
        // the first component is ignored
        r_value = data[row_offset + (row_bytes / 4) + x];
        g_value = data[row_offset + (2 * row_bytes / 4) + x];
        b_value = data[row_offset + (3 * row_bytes / 4) + x];

      } else if (args.header.component_size == 5) {
        // xrgb1555. see decode_color_image for an explanation of the bit
        // manipulation below
        uint16_t value = bswap16(*reinterpret_cast<const uint16_t*>(&data[row_offset + 2 * x]));
        r_value = ((value >> 7) & 0xF8) | ((value >> 12) & 0x07);
        g_value = ((value >> 2) & 0xF8) | ((value >> 7) & 0x07);
        b_value = ((value << 3) & 0xF8) | ((value >> 2) & 0x07);

      } else {
        throw logic_error("unimplemented channel width");
      }

      this->write_canvas_pixel(x + args.dest_rect.x1, y + args.dest_rect.y1,
          r_value, g_value, b_value);
    }
  }
}



// QuickTime embedded file support

Color8 QuickDrawEngine::decode_rgb555(uint16_t color) {
  // color is like 0rrrrrgg gggbbbbb
  // to extend an rgb555 color into 24-bit colorspace, we just echo the most-
  // significant bits again. so (for example) r1r2r3r4r5 => r1r2r3r4r5r1r2r3
  color &= 0x7FFF;
  return {
    static_cast<uint8_t>((color >> 7) | (color >> 12)),
    static_cast<uint8_t>((color >> 2) | ((color >> 7) & 7)),
    static_cast<uint8_t>((color << 3) | ((color >> 2) & 7)),
  };
}

Image QuickDrawEngine::pict_decode_smc(
    const PictQuickTimeImageDescription& desc,
    const vector<Color>& clut,
    const string& data) {
  if (data.size() < 4) {
    throw runtime_error("smc-encoded image too small for header");
  }

  uint8_t color_index_cache2[0x100][2];
  uint8_t color_index_cache2_pos = 0;
  uint8_t color_index_cache4[0x100][4];
  uint8_t color_index_cache4_pos = 0;
  uint8_t color_index_cache8[0x100][8];
  uint8_t color_index_cache8_pos = 0;

  StringReader r(data.data(), data.size());
  r.get_u8(); // skip flags byte
  uint32_t encoded_size = r.get_u24r();
  if (encoded_size != data.size()) {
    throw runtime_error("smc-encoded image has incorrect size header");
  }

  Image ret(desc.width, desc.height, true);
  ret.clear(0x00, 0x00, 0x00, 0x00);
  size_t prev_x2, prev_y2;
  size_t prev_x1, prev_y1;
  size_t x = 0, y = 0;
  auto advance_block = [&]() {
    if (y >= ret.get_height()) {
      throw runtime_error("smc decoder advanced beyond end of output image");
    }
    prev_x2 = prev_x1;
    prev_y2 = prev_y1;
    prev_x1 = x;
    prev_y1 = y;
    x += 4;
    if (x >= ret.get_width()) {
      y += 4;
      x = 0;
    }
  };

  auto write_color = [&](size_t x, size_t y, uint8_t color_index) {
    const auto& color_entry = clut.at(color_index);
    try {
      ret.write_pixel(x, y, color_entry.r / 0x101, color_entry.g / 0x101,
          color_entry.b / 0x101, 0xFF);
    } catch (const runtime_error&) { }
  };

  while (!r.eof()) {
    uint8_t opcode = r.get_u8();
    if ((opcode & 0xF0) == 0xF0) {
      throw runtime_error("smc-encoded contains opcode 0xF0");
    }
    switch (opcode & 0xE0) {
      case 0x00: { // skip blocks
        uint8_t num_blocks = ((opcode & 0x10) ? r.get_u8() : (opcode & 0x0F)) + 1;
        for (size_t z = 0; z < num_blocks; z++) {
          advance_block();
        }
        break;
      }

      case 0x20: { // repeat last block
        uint8_t num_blocks = ((opcode & 0x10) ? r.get_u8() : (opcode & 0x0F)) + 1;
        for (size_t z = 0; z < num_blocks; z++) {
          ret.blit(ret, x, y, 4, 4, prev_x1, prev_y1);
          advance_block();
        }
        break;
      }

      case 0x40: { // repeat previous pair of blocks
        uint16_t num_blocks = (((opcode & 0x10) ? r.get_u8() : (opcode & 0x0F)) + 1) * 2;
        for (size_t z = 0; z < num_blocks; z++) {
          ret.blit(ret, x, y, 4, 4, prev_x2, prev_y2);
          advance_block();
        }
        break;
      }

      case 0x60: { // 1-color encoding
        uint8_t num_blocks = ((opcode & 0x10) ? r.get_u8() : (opcode & 0x0F)) + 1;
        uint8_t color_index = r.get_u8();
        const auto& color = clut.at(color_index);
        uint64_t r = color.r / 0x0101;
        uint64_t g = color.g / 0x0101;
        uint64_t b = color.b / 0x0101;
        for (size_t z = 0; z < num_blocks; z++) {
          ret.fill_rect(x, y, 4, 4, r, g, b, 0xFF);
          advance_block();
        }
        break;
      }

      case 0x80: { // 2-color encoding
        uint8_t num_blocks = (opcode & 0x0F) + 1;
        uint8_t color_indexes[2];
        if ((opcode & 0xF0) == 0x80) {
          color_indexes[0] = r.get_u8();
          color_indexes[1] = r.get_u8();
          memcpy(color_index_cache2[color_index_cache2_pos++], color_indexes, sizeof(color_indexes));
        } else { // 0x90
          uint8_t cache_index = r.get_u8();
          memcpy(color_indexes, color_index_cache2[cache_index], sizeof(color_indexes));
        }
        for (size_t z = 0; z < num_blocks; z++) {
          uint8_t top_colors = r.get_u8();
          uint8_t bottom_colors = r.get_u8();
          for (size_t yy = 0; yy < 2; yy++) {
            for (size_t xx = 0; xx < 4; xx++) {
              write_color(x + xx, y + yy,
                  color_indexes[!!(top_colors & (0x80 >> (yy * 4 + xx)))]);
            }
          }
          for (size_t yy = 0; yy < 2; yy++) {
            for (size_t xx = 0; xx < 4; xx++) {
              write_color(x + xx, y + 2 + yy,
                  color_indexes[!!(bottom_colors & (0x80 >> (yy * 4 + xx)))]);
            }
          }
          advance_block();
        }
        break;
      }

      case 0xA0: { // 4-color encoding
        uint8_t num_blocks = (opcode & 0x0F) + 1;
        uint8_t color_indexes[4];
        if ((opcode & 0xF0) == 0xA0) {
          for (size_t z = 0; z < 4; z++) {
            color_indexes[z] = r.get_u8();
          }
          memcpy(color_index_cache4[color_index_cache4_pos++], color_indexes, sizeof(color_indexes));
        } else { // 0xB0
          uint8_t cache_index = r.get_u8();
          memcpy(color_indexes, color_index_cache4[cache_index], sizeof(color_indexes));
        }
        for (size_t z = 0; z < num_blocks; z++) {
          for (size_t yy = 0; yy < 4; yy++) {
            uint8_t row_colors = r.get_u8();
            for (size_t xx = 0; xx < 4; xx++) {
              write_color(x + xx, y + yy,
                  color_indexes[(row_colors >> (6 - (2 * xx))) & 0x03]);
            }
          }
          advance_block();
        }
        break;
      }

      case 0xC0: { // 8-color encoding
        uint8_t num_blocks = (opcode & 0x0F) + 1;
        uint8_t color_indexes[8];
        if ((opcode & 0xF0) == 0xC0) {
          for (size_t z = 0; z < 8; z++) {
            color_indexes[z] = r.get_u8();
          }
          memcpy(color_index_cache8[color_index_cache8_pos++], color_indexes, sizeof(color_indexes));
        } else { // 0xD0
          uint8_t cache_index = r.get_u8();
          memcpy(color_indexes, color_index_cache8[cache_index], sizeof(color_indexes));
        }

        for (size_t z = 0; z < num_blocks; z++) {
          uint64_t block_colors = r.get_u48r();
          // for some reason we have to shuffle the bits around like so:
          // read: 0000 1111 2222 3333 4444 5555 6666 7777 8888 9999 AAAA BBBB
          // used: 0000 1111 2222 4444 5555 6666 8888 9999 AAAA 3333 7777 BBBB
          // what were you thinking, sean callahan?
          block_colors =
              (block_colors         & 0xFFF00000000F) |
              ((block_colors << 4)  & 0x000FFF000000) |
              ((block_colors << 8)  & 0x000000FFF000) |
              ((block_colors >> 24) & 0x000000000F00) |
              ((block_colors >> 12) & 0x0000000000F0);
          for (size_t yy = 0; yy < 4; yy++) {
            for (size_t xx = 0; xx < 4; xx++) {
              write_color(x + xx, y + yy,
                  color_indexes[(block_colors >> (45 - (yy * 12) - (xx * 3))) & 0x07]);
            }
          }
          advance_block();
        }
        break;
      }

      case 0xE0: { // 16-color encoding
        uint8_t num_blocks = (opcode & 0x0F) + 1;
        for (size_t z = 0; z < num_blocks; z++) {
          for (size_t yy = 0; yy < 4; yy++) {
            for (size_t xx = 0; xx < 4; xx++) {
              write_color(x + xx, y + yy, r.get_u8());
            }
          }
          advance_block();
        }
        break;
      }
    }
  }

  return ret;
}

Image QuickDrawEngine::pict_decode_rpza(
    const PictQuickTimeImageDescription& desc,
    const vector<Color>& clut,
    const string& data) {
  if (data.size() < 4) {
    throw runtime_error("rpza-encoded image too small for header");
  }

  StringReader r(data.data(), data.size());
  if (r.get_u8() != 0xE1) {
    throw runtime_error("rpza-encoded image does not start with frame command");
  }
  uint32_t encoded_size = r.get_u24r();
  if (encoded_size != data.size()) {
    throw runtime_error("rpza-encoded image has incorrect size header");
  }

  Image ret(desc.width, desc.height, true);
  ret.clear(0x00, 0x00, 0x00, 0x00);
  size_t x = 0, y = 0;
  auto advance_block = [&]() {
    if (y >= ret.get_height()) {
      throw runtime_error("rpza decoder advanced beyond end of output image");
    }
    x += 4;
    if (x >= ret.get_width()) {
      y += 4;
      x = 0;
    }
  };

  auto decode_four_color_blocks = [&](uint16_t color_a, uint16_t color_b, uint8_t num_blocks) {
    Color8 c[4];
    c[3] = this->decode_rgb555(color_a);
    c[0] = this->decode_rgb555(color_b);
    c[1] = {static_cast<uint8_t>((11 * c[3].r + 21 * c[0].r) / 32),
            static_cast<uint8_t>((11 * c[3].g + 21 * c[0].g) / 32),
            static_cast<uint8_t>((11 * c[3].b + 21 * c[0].b) / 32)};
    c[2] = {static_cast<uint8_t>((21 * c[3].r + 11 * c[0].r) / 32),
            static_cast<uint8_t>((21 * c[3].g + 11 * c[0].g) / 32),
            static_cast<uint8_t>((21 * c[3].b + 11 * c[0].b) / 32)};
    for (uint8_t z = 0; z < num_blocks; z++) {
      for (size_t yy = 0; yy < 4; yy++) {
        uint8_t row_indexes = r.get_u8();
        for (size_t xx = 0; xx < 4; xx++) {
          const Color8& color = c[(row_indexes >> (6 - (2 * xx))) & 3];
          try {
            ret.write_pixel(x + xx, y + yy, color.r, color.g, color.b, 0xFF);
          } catch (const runtime_error&) { }
        }
      }
      advance_block();
    }
  };

  while (!r.eof()) {
    uint8_t opcode = r.get_u8();
    if (opcode & 0x80) {
      uint8_t block_count = (opcode & 0x1F) + 1;
      switch (opcode & 0x60) {
        case 0x00: // skip blocks
          for (uint8_t z = 0; z < block_count; z++) {
            advance_block();
          }
          break;
        case 0x20: { // single color
          auto color = decode_rgb555(r.get_u16r());
          for (uint8_t z = 0; z < block_count; z++) {
            ret.fill_rect(x, y, 4, 4, color.r, color.g, color.b, 0xFF);
            advance_block();
          }
          break;
        }
        case 0x40: { // 4 colors
          uint16_t color_a = r.get_u16r();
          uint16_t color_b = r.get_u16r();
          decode_four_color_blocks(color_a, color_b, block_count);
          break;
        }
        case 0x60:
          throw runtime_error("rpza-encoded image uses command 60");
      }
    } else {
      uint16_t color_a = (static_cast<uint16_t>(opcode) << 8) | r.get_u8();
      uint8_t subopcode = r.get_u8(false);
      if (subopcode & 0x80) { // 0x40, but for only one block
        uint16_t color_b = r.get_u16r();
        decode_four_color_blocks(color_a, color_b, 1);
      } else { // 16 different colors
        for (size_t yy = 0; yy < 4; yy++) {
          for (size_t xx = 0; xx < 4; xx++) {
            Color8 color = decode_rgb555((xx + yy == 0) ? color_a : r.get_u16r());
            ret.write_pixel(x + xx, y + yy, color.r, color.g, color.b, 0xFF);
          }
        }
        advance_block();
      }
    }
  }

  return ret;
}

void QuickDrawEngine::pict_write_quicktime_data(StringReader& r, uint16_t opcode) {
  bool is_compressed = !(opcode & 0x01);

  uint32_t matte_size;
  if (!is_compressed) {
    PictUncompressedQuickTimeArgs args = r.get<PictUncompressedQuickTimeArgs>();
    args.byteswap();
    matte_size = args.matte_size;
  } else {
    // get the compressed data header and check for unsupported fancy stuff
    PictCompressedQuickTimeArgs args = r.get<PictCompressedQuickTimeArgs>();
    args.byteswap();
    matte_size = args.matte_size;
    if (args.mask_region_size) {
      throw runtime_error("compressed QuickTime data includes a mask region");
    }
  }

  // TODO: in the future if we ever support matte images, we'll have to read the
  // header data for them here. in both the compressed and uncompressed cases,
  // these fields are present if matte_size != 0:
  // - matte_image_description
  // - matte_data
  if (matte_size) {
    // the next header is always word-aligned, so if the matte image is an odd
    // number of bytes, round up
    fprintf(stderr, "warning: skipping matte image (%u bytes) from QuickTime data\n", matte_size);
    r.go((r.where() + matte_size + 1) & ~1);
  }

  if (is_compressed) {
    // TODO: this is where we would read the mask region, if we ever support it

    // get the image description and check for unsupported fancy stuff
    PictQuickTimeImageDescription desc = r.get<PictQuickTimeImageDescription>();
    desc.byteswap();
    if (desc.frame_count != 1) {
      throw runtime_error("compressed QuickTime data includes zero or multiple frames");
    }

    // if clut_id == 0, a struct color_table immediately follows the image description
    vector<Color> clut;
    if (desc.clut_id == 0) {
      ColorTable clut_header = r.get<ColorTable>();
      clut_header.byteswap_header();
      while (clut.size() < clut_header.get_num_entries()) {
        ColorTableEntry entry = r.get<ColorTableEntry>();
        entry.byteswap();
        clut.push_back(entry.c);
      }
    } else if (desc.clut_id != 0xFFFF) {
      clut = this->port->read_clut(desc.clut_id);
    }

    // read the encoded image data
    string encoded_data = r.read(desc.data_size);

    // find the appropriate handler, if it's implemented
    Image decoded(0, 0);
    if (desc.codec == 0x736D6320) { // kGraphicsCodecType
      decoded = this->pict_decode_smc(desc, clut, encoded_data);
    } else if (desc.codec == 0x72707A61) { // kVideoCodecType
      decoded = this->pict_decode_rpza(desc, clut, encoded_data);
    } else if (desc.codec == 0x67696620) { // kGIFCodecType
      throw pict_contains_undecodable_quicktime("gif", move(encoded_data));
    } else if (desc.codec == 0x6A706567) { // kJPEGCodecType
      throw pict_contains_undecodable_quicktime("jpeg", move(encoded_data));
    } else if (desc.codec == 0x6B706364) { // kPhotoCDCodecType
      throw pict_contains_undecodable_quicktime("pcd", move(encoded_data));
    } else if (desc.codec == 0x706E6720) { // kPNGCodecType
      throw pict_contains_undecodable_quicktime("png", move(encoded_data));
    } else if (desc.codec == 0x74676120) { // kTargaCodecType
      throw pict_contains_undecodable_quicktime("tga", move(encoded_data));
    } else if (desc.codec == 0x74696666) { // kTIFFCodecType
      throw pict_contains_undecodable_quicktime("tiff", move(encoded_data));
    } else {
      throw runtime_error(string_printf("compressed QuickTime data uses codec %08" PRIX32, desc.codec));
    }

    if (decoded.get_width() != this->port->width() || decoded.get_height() != this->port->height()) {
      throw runtime_error("decoded QuickTIme image dimensions do not match port dimensions");
    }

    this->port->blit(decoded, 0, 0, decoded.get_width(), decoded.get_height());

  } else {
    // "uncompressed" QuickTime data has a subordinate opcode at this position
    // that just renders the data directly. according to the docs, this must
    // always be a CopyBits opcode; unclear if this is actually enforced by
    // QuickDraw though (and if we need to support more than 9x opcodes here)
    uint16_t subopcode = r.get_u16r();
    if (subopcode == 0x0098 || subopcode == 0x0099) {
      this->pict_copy_bits_indexed_color(r, subopcode);
    } else if (subopcode == 0x009A || subopcode == 0x009B) {
      this->pict_packed_copy_bits_direct_color(r, subopcode);
    } else {
      throw runtime_error(string_printf(
          "uncompressed QuickTime data uses non-CopyBits subopcode %hu", subopcode));
    }
  }
}



// opcode index

const vector<void(QuickDrawEngine::*)(StringReader&, uint16_t)> QuickDrawEngine::render_functions({
  &QuickDrawEngine::pict_skip_0,                         // 0000: no operation (args: 0)
  &QuickDrawEngine::pict_set_clipping_region,            // 0001: clipping region (args: region)
  &QuickDrawEngine::pict_set_background_pattern,         // 0002: background pattern (args: ?8)
  &QuickDrawEngine::pict_set_font_number,                // 0003: text font number (args: u16)
  &QuickDrawEngine::pict_set_font_style_flags,           // 0004: text font style (args: u8)
  &QuickDrawEngine::pict_set_text_source_mode,           // 0005: text source mode (args: u16)
  &QuickDrawEngine::pict_set_text_extra_space,           // 0006: extra space (args: u32)
  &QuickDrawEngine::pict_set_pen_size,                   // 0007: pen size (args: point)
  &QuickDrawEngine::pict_set_pen_mode,                   // 0008: pen mode (args: u16)
  &QuickDrawEngine::pict_set_pen_pattern,                // 0009: pen pattern (args: ?8)
  &QuickDrawEngine::pict_set_fill_pattern,               // 000A: fill pattern (args: ?8)
  &QuickDrawEngine::pict_set_oval_size,                  // 000B: oval size (args: point)
  &QuickDrawEngine::pict_set_origin_dh_dv,               // 000C: set origin dh/dv (args: u16, u16)
  &QuickDrawEngine::pict_set_text_size,                  // 000D: text size (args: u16)
  &QuickDrawEngine::pict_set_foreground_color32,         // 000E: foreground color (args: u32)
  &QuickDrawEngine::pict_set_background_color32,         // 000F: background color (args: u32)
  &QuickDrawEngine::pict_set_text_ratio,                 // 0010: text ratio? (args: point numerator, point denominator)
  &QuickDrawEngine::pict_set_version,                    // 0011: version (args: u8)
  &QuickDrawEngine::pict_set_background_pixel_pattern,   // 0012: background pixel pattern (missing in v1) (args: ?)
  &QuickDrawEngine::pict_set_pen_pixel_pattern,          // 0013: pen pixel pattern (missing in v1) (args: ?)
  &QuickDrawEngine::pict_set_fill_pixel_pattern,         // 0014: fill pixel pattern (missing in v1) (args: ?)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0015: fractional pen position (missing in v1) (args: u16 low word of fixed)
  &QuickDrawEngine::pict_set_text_nonspace_extra_width,  // 0016: added width for nonspace characters (missing in v1) (args: u16)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0017: reserved (args: indeterminate)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0018: reserved (args: indeterminate)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0019: reserved (args: indeterminate)
  &QuickDrawEngine::pict_set_foreground_color,           // 001A: foreground color (missing in v1) (args: rgb48)
  &QuickDrawEngine::pict_set_background_color,           // 001B: background color (missing in v1) (args: rgb48)
  &QuickDrawEngine::pict_set_highlight_mode_flag,        // 001C: highlight mode flag (missing in v1) (args: 0)
  &QuickDrawEngine::pict_set_highlight_color,            // 001D: highlight color (missing in v1) (args: rgb48)
  &QuickDrawEngine::pict_set_default_highlight_color,    // 001E: use default highlight color (missing in v1) (args: 0)
  &QuickDrawEngine::pict_set_op_color,                   // 001F: color (missing in v1) (args: rgb48)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0020: line (args: point, point)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0021: line from (args: point)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0022: short line (args: point, s8 dh, s8 dv)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0023: short line from (args: s8 dh, s8 dv)
  &QuickDrawEngine::pict_skip_var16,                     // 0024: reserved (args: u16 data length, u8[] data)
  &QuickDrawEngine::pict_skip_var16,                     // 0025: reserved (args: u16 data length, u8[] data)
  &QuickDrawEngine::pict_skip_var16,                     // 0026: reserved (args: u16 data length, u8[] data)
  &QuickDrawEngine::pict_skip_var16,                     // 0027: reserved (args: u16 data length, u8[] data)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0028: long text (args: point, u8 count, char[] text)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0029: dh text (args: u8 dh, u8 count, char[] text)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 002A: dv text (args: u8 dv, u8 count, char[] text)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 002B: dh/dv text (args: u8 dh, u8 dv, u8 count, char[] text)
  &QuickDrawEngine::pict_set_font_number_and_name,       // 002C: font name (missing in v1) (args: u16 length, u16 old font id, u8 name length, char[] name)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 002D: line justify (missing in v1) (args: u16 data length, fixed interchar spacing, fixed total extra space)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 002E: glyph state (missing in v1) (u16 data length, u8 outline, u8 preserve glyph, u8 fractional widths, u8 scaling disabled)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 002F: reserved (args: u16 data length, u8[] data)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0030: frame rect (args: rect)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0031: paint rect (args: rect)
  &QuickDrawEngine::pict_erase_rect,                     // 0032: erase rect (args: rect)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0033: invert rect (args: rect)
  &QuickDrawEngine::pict_fill_rect,                      // 0034: fill rect (args: rect)
  &QuickDrawEngine::pict_skip_8,                         // 0035: reserved (args: rect)
  &QuickDrawEngine::pict_skip_8,                         // 0036: reserved (args: rect)
  &QuickDrawEngine::pict_skip_8,                         // 0037: reserved (args: rect)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0038: frame same rect (args: 0)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0039: paint same rect (args: 0)
  &QuickDrawEngine::pict_erase_last_rect,                // 003A: erase same rect (args: 0)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 003B: invert same rect (args: 0)
  &QuickDrawEngine::pict_fill_last_rect,                 // 003C: fill same rect (args: 0)
  &QuickDrawEngine::pict_skip_0,                         // 003D: reserved (args: 0)
  &QuickDrawEngine::pict_skip_0,                         // 003E: reserved (args: 0)
  &QuickDrawEngine::pict_skip_0,                         // 003F: reserved (args: 0)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0040: frame rrect (args: rect)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0041: paint rrect (args: rect)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0042: erase rrect (args: rect)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0043: invert rrect (args: rect)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0044: fill rrect (args: rect)
  &QuickDrawEngine::pict_skip_8,                         // 0045: reserved (args: rect)
  &QuickDrawEngine::pict_skip_8,                         // 0046: reserved (args: rect)
  &QuickDrawEngine::pict_skip_8,                         // 0047: reserved (args: rect)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0048: frame same rrect (args: 0)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0049: paint same rrect (args: 0)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 004A: erase same rrect (args: 0)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 004B: invert same rrect (args: 0)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 004C: fill same rrect (args: 0)
  &QuickDrawEngine::pict_skip_0,                         // 004D: reserved (args: 0)
  &QuickDrawEngine::pict_skip_0,                         // 004E: reserved (args: 0)
  &QuickDrawEngine::pict_skip_0,                         // 004F: reserved (args: 0)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0050: frame oval (args: rect)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0051: paint oval (args: rect)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0052: erase oval (args: rect)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0053: invert oval (args: rect)
  &QuickDrawEngine::pict_fill_oval,                      // 0054: fill oval (args: rect)
  &QuickDrawEngine::pict_skip_8,                         // 0055: reserved (args: rect)
  &QuickDrawEngine::pict_skip_8,                         // 0056: reserved (args: rect)
  &QuickDrawEngine::pict_skip_8,                         // 0057: reserved (args: rect)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0058: frame same oval (args: 0)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0059: paint same oval (args: 0)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 005A: erase same oval (args: 0)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 005B: invert same oval (args: 0)
  &QuickDrawEngine::pict_fill_last_oval,                 // 005C: fill same oval (args: 0)
  &QuickDrawEngine::pict_skip_0,                         // 005D: reserved (args: 0)
  &QuickDrawEngine::pict_skip_0,                         // 005E: reserved (args: 0)
  &QuickDrawEngine::pict_skip_0,                         // 005F: reserved (args: 0)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0060: frame arc (args: rect, u16 start angle, u16 arc angle)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0061: paint arc (args: rect, u16 start angle, u16 arc angle)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0062: erase arc (args: rect, u16 start angle, u16 arc angle)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0063: invert arc (args: rect, u16 start angle, u16 arc angle)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0064: fill arc (args: rect, u16 start angle, u16 arc angle)
  &QuickDrawEngine::pict_skip_12,                        // 0065: reserved (args: rect, u16 start angle, u16 arc angle)
  &QuickDrawEngine::pict_skip_12,                        // 0066: reserved (args: rect, u16 start angle, u16 arc angle)
  &QuickDrawEngine::pict_skip_12,                        // 0067: reserved (args: rect, u16 start angle, u16 arc angle)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0068: frame same arc (args: rect)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0069: paint same arc (args: rect)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 006A: erase same arc (args: rect)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 006B: invert same arc (args: rect)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 006C: fill same arc (args: rect)
  &QuickDrawEngine::pict_skip_8,                         // 006D: reserved (args: rect)
  &QuickDrawEngine::pict_skip_8,                         // 006E: reserved (args: rect)
  &QuickDrawEngine::pict_skip_8,                         // 006F: reserved (args: rect)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0070: frame poly (args: polygon)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0071: paint poly (args: polygon)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0072: erase poly (args: polygon)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0073: invert poly (args: polygon)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0074: fill poly (args: polygon)
  &QuickDrawEngine::pict_skip_var16,                     // 0075: reserved (args: polygon)
  &QuickDrawEngine::pict_skip_var16,                     // 0076: reserved (args: polygon)
  &QuickDrawEngine::pict_skip_var16,                     // 0077: reserved (args: polygon)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0078: frame same poly (args: 0)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0079: paint same poly (args: 0)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 007A: erase same poly (args: 0)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 007B: invert same poly (args: 0)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 007C: fill same poly (args: 0)
  &QuickDrawEngine::pict_skip_0,                         // 007D: reserved (args: 0)
  &QuickDrawEngine::pict_skip_0,                         // 007E: reserved (args: 0)
  &QuickDrawEngine::pict_skip_0,                         // 007F: reserved (args: 0)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0080: frame region (args: region)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0081: paint region (args: region)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0082: erase region (args: region)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0083: invert region (args: region)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0084: fill region (args: region)
  &QuickDrawEngine::pict_skip_var16,                     // 0085: reserved (args: region)
  &QuickDrawEngine::pict_skip_var16,                     // 0086: reserved (args: region)
  &QuickDrawEngine::pict_skip_var16,                     // 0087: reserved (args: region)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0088: frame same region (args: 0)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 0089: paint same region (args: 0)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 008A: erase same region (args: 0)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 008B: invert same region (args: 0)
  &QuickDrawEngine::pict_unimplemented_opcode,           // 008C: fill same region (args: 0)
  &QuickDrawEngine::pict_skip_0,                         // 008D: reserved (args: 0)
  &QuickDrawEngine::pict_skip_0,                         // 008E: reserved (args: 0)
  &QuickDrawEngine::pict_skip_0,                         // 008F: reserved (args: 0)
  &QuickDrawEngine::pict_copy_bits_indexed_color,        // 0090: copybits into rect (args: struct)
  &QuickDrawEngine::pict_copy_bits_indexed_color,        // 0091: copybits into region (args: struct)
  &QuickDrawEngine::pict_skip_var16,                     // 0092: reserved (args: u16 data length, u8[] data)
  &QuickDrawEngine::pict_skip_var16,                     // 0093: reserved (args: u16 data length, u8[] data)
  &QuickDrawEngine::pict_skip_var16,                     // 0094: reserved (args: u16 data length, u8[] data)
  &QuickDrawEngine::pict_skip_var16,                     // 0095: reserved (args: u16 data length, u8[] data)
  &QuickDrawEngine::pict_skip_var16,                     // 0096: reserved (args: u16 data length, u8[] data)
  &QuickDrawEngine::pict_skip_var16,                     // 0097: reserved (args: u16 data length, u8[] data)
  &QuickDrawEngine::pict_copy_bits_indexed_color,        // 0098: packed indexed color or monochrome copybits into rect (args: struct)
  &QuickDrawEngine::pict_copy_bits_indexed_color,        // 0099: packed indexed color or monochrome copybits into region (args: struct)
  &QuickDrawEngine::pict_packed_copy_bits_direct_color,  // 009A: packed direct color copybits into rect (missing in v1) (args: struct)
  &QuickDrawEngine::pict_packed_copy_bits_direct_color,  // 009B: packed direct color copybits into region (missing in v1) (args: ?)
  &QuickDrawEngine::pict_skip_var16,                     // 009C: reserved (args: u16 data length, u8[] data)
  &QuickDrawEngine::pict_skip_var16,                     // 009D: reserved (args: u16 data length, u8[] data)
  &QuickDrawEngine::pict_skip_var16,                     // 009E: reserved (args: u16 data length, u8[] data)
  &QuickDrawEngine::pict_skip_var16,                     // 009F: reserved (args: u16 data length, u8[] data)
  &QuickDrawEngine::pict_skip_2,                         // 00A0: short comment (args: u16 kind)
  &QuickDrawEngine::pict_skip_long_comment,              // 00A1: long comment (args: u16 kind, u16 length, char[] data)
});

void QuickDrawEngine::render_pict(const void* vdata, size_t size) {
  if (size < sizeof(PictHeader)) {
    throw runtime_error("pict too small for header");
  }

  StringReader r(vdata, size);
  PictHeader header = r.get<PictHeader>();
  header.byteswap();

  // if the pict header is all zeroes, assume this is a pict file with a
  // 512-byte header that needs to be skipped
  if (header.size == 0 && header.bounds.x1 == 0 && header.bounds.y1 == 0 &&
      header.bounds.x2 == 0 && header.bounds.y2 == 0 && size > 0x200) {
    r.go(0x200);
    header = r.get<PictHeader>();
    header.byteswap();
  }

  this->pict_bounds = header.bounds;
  this->pict_oval_size = Point(0, 0);
  this->pict_origin = Point(0, 0);
  this->pict_text_ratio_numerator = Point(1, 1);
  this->pict_text_ratio_denominator = Point(1, 1);
  this->pict_version = 1;
  this->pict_highlight_flag = false;
  this->pict_last_rect = Rect(0, 0, 0, 0);

  while (!r.eof()) {
    // in v2 pictures, opcodes are word-aligned
    if ((this->pict_version == 2) && (r.where() & 1)) {
      r.get_u8();
    }

    uint16_t opcode = (this->pict_version == 1) ? r.get_u8() : r.get_u16r();
    if (opcode < this->render_functions.size()) {
      auto fn = this->render_functions[opcode];
      (this->*fn)(r, opcode);

    } else if (opcode <= 0xAF) { // args: u16 len, u8[] data
      this->pict_skip_var16(r, opcode);

    } else if (opcode <= 0xCF) { // args: 0
      // nop

    } else if (opcode <= 0xFE) { // args: u32 len, u8[] data
      this->pict_skip_var32(r, opcode);

    } else if (opcode == 0xFF) { // args: 0
      break; // end picture

    } else if (opcode <= 0x01FF) { // args: 2
      this->pict_skip_2(r, opcode);

    } else if (opcode <= 0x02FE) { // args: 4
      r.go(r.where() + 4);

    } else if (opcode == 0x02FF) { // args: 2
      // nop (essentially) because we look ahead in the 0011 implementation

    } else if (opcode <= 0x0BFF) { // args: 22
      r.go(r.where() + 22);

    } else if (opcode == 0x0C00) { // args: header
      // currently we don't do anything with tyhe data in this subheader, so
      // just check that its version make sense and ignore it
      PictSubheader h = r.get<PictSubheader>();
      if ((bswap32(h.v2.version) != 0xFFFFFFFF) && (bswap16(h.v2e.version) != 0xFFFE)) {
        throw runtime_error(string_printf("subheader has incorrect version (%08X or %04hX)",
            bswap32(h.v2.version), bswap16(h.v2e.version)));
      }

    } else if (opcode <= 0x7EFF) { // args: 24
      r.go(r.where() + 24);

    } else if (opcode <= 0x7FFF) { // args: 254
      r.go(r.where() + 254);

    } else if (opcode <= 0x80FF) { // args: 0
      // nop

    } else if (opcode <= 0x81FF) { // args: u32 len, u8[] data
      this->pict_skip_var32(r, opcode);

    } else if ((opcode & 0xFFFE) == 0x8200) { // args: pict_compressed_quicktime_args or pict_uncompressed_quicktime_args
      this->pict_write_quicktime_data(r, opcode);
      // TODO: it appears that these opcodes always just end rendering, since
      // some PICTs that include them have rendering opcodes afterward that
      // appear to do backup things, like render text saying "You need QuickTime
      // to see this picture". So we just end rendering immediately, which seems
      // correct, in practice, but I haven't been able to verify this via
      // documentation.
      break;

    } else { // args: u32 len, u8[] data
      this->pict_skip_var32(r, opcode);
    }
  }
}
