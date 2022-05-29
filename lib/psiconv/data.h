/*
    data.h - Part of psiconv, a PSION 5 file formats converter
    Copyright (c) 1999-2014  Frodo Looijaard <frodo@frodo.looijaard.name>       

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* This file contains the declarations of all types that are used to
   represent the Psion files. Variables of these types are written by the
   parse routines, and read by the generation routines. 
   
   Mostly, the data structures reflect the file format documentation,
   as included in the formats directory. When in doubt, refer there. */

#ifndef PSICONV_DATA_H
#define PSICONV_DATA_H

#include <psiconv/general.h>
#include <psiconv/list.h>

/* All types which end on _t are plain types; all other types are pointers
   to structs. */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Forward declaration (for psiconv_embedded_object_section) */
typedef struct psiconv_file_s *psiconv_file;


/* Enums and simple types */


/* Floating point number representation */
typedef double psiconv_float_t;

/* The supported file types. */
typedef enum psiconv_file_type {
  psiconv_unknown_file,
  psiconv_word_file,
  psiconv_texted_file,
  psiconv_mbm_file,
  psiconv_sketch_file,
  psiconv_clipart_file,
  psiconv_sheet_file
} psiconv_file_type_t;
  
/* String representation. A string is an array of UCS2 characters, terminated
   by a 0. So they are just like normal C strings, except that they are built
   of psiconv_ucs2 elements instead of char elements.
   The psiconv_ucs2 type holds 16 bits; see unicode.h for more information. */
typedef psiconv_ucs2 *psiconv_string_t;

/* Represent lengths (in centimeters) and sizes (in points).
   In the Psion file, these are identical; but we translate them to more
   human-readable quantities */
typedef float psiconv_length_t;    /* For offsets in centimeters */
typedef float psiconv_size_t;      /* For sizes in points */

/* Represent booleans. As false is zero in the enum, you can still do things
   like { if (test) ... } instead of { if (test == psiconv_bool_true) ... }.
   Choose whatever style suits you best. */
typedef enum psiconv_bool
{
  psiconv_bool_false,
  psiconv_bool_true
} psiconv_bool_t;

/* Some kind of three-valued boolean, used at several places. */
typedef enum psiconv_triple
{
  psiconv_triple_on,
  psiconv_triple_off,
  psiconv_triple_auto
} psiconv_triple_t;

/* Text can be in superscript or subscript or neither, but never both
   superscript and subscript at once. Also, super-superscript and things
   like that do not exist in the Psion world. */
typedef enum psiconv_super_sub
{ psiconv_normalscript,
  psiconv_superscript,
  psiconv_subscript
} psiconv_super_sub_t;

/* Horizontal justification. */
typedef enum psiconv_justify_hor
{ psiconv_justify_left, 
  psiconv_justify_centre, 
  psiconv_justify_right, 
  psiconv_justify_full 
} psiconv_justify_hor_t;

/* Vertical justification. */
typedef enum psiconv_justify_ver
{ psiconv_justify_top,
  psiconv_justify_middle,
  psiconv_justify_bottom
} psiconv_justify_ver_t;

/* Borders around text fields. */
typedef enum psiconv_border_kind
{ psiconv_border_none,            /* No border */
  psiconv_border_solid,           /* Single line */
  psiconv_border_double,          /* Double line */
  psiconv_border_dotted,          /* Dotted line: . . . . . */
  psiconv_border_dashed,          /* Dashed line: _ _ _ _ _ */
  psiconv_border_dotdashed,       /* Dotted/dashed line: _ . _ . _ */
  psiconv_border_dotdotdashed     /* Dotted/dashed line: . . _ . . _ */
} psiconv_border_kind_t;

/* Though each printer driver has its own fonts for printing, they are
   represented on the Psion screen by a few built-in fonts. */
typedef enum psiconv_screenfont
{
  psiconv_font_misc,          /* Nonproportional symbols, like Wingbat? */
  psiconv_font_sansserif,     /* Proportional sans-serif, like Arial */
  psiconv_font_nonprop,       /* Nonproportional, like Courier */
  psiconv_font_serif          /* Proportional serifed, like Times */
} psiconv_screenfont_t;


/* The kind of tab. Note that decimal tabs are not supported by the Psion. */
typedef enum psiconv_tab_kind
{
  psiconv_tab_left,      /* Left tab */
  psiconv_tab_centre,    /* Centre tab */
  psiconv_tab_right      /* Right tab */
} psiconv_tab_kind_t;

/* When text has to be replaced, the kind of replacement to do
   (not yet implemented!) */
typedef enum psiconv_replacement_type
{
  psiconv_replace_time,
  psiconv_replace_date,
  psiconv_replace_pagenr,
  psiconv_replace_nr_of_pages,
  psiconv_replace_filename
} psiconv_replacement_type_t;


/* Here starts the struct definitions */

/* The color of a single pixel, in RGB format.
   Black: 0x00 0x00 0x00 
   White: 0xff 0xff 0xff */
typedef struct psiconv_color_s
{
  psiconv_u8 red;
  psiconv_u8 green;
  psiconv_u8 blue;
} * psiconv_color;


/* Complete font information: both a printer font and a corresponding screen
   font to display it. */
typedef struct psiconv_font_s
{
  psiconv_string_t name;             /* Printer font */
  psiconv_screenfont_t screenfont;   /* Screen font */
} *psiconv_font;

/* Complete border information */
typedef struct psiconv_border_s
{ 
  psiconv_border_kind_t kind;   /* Border kind */
  psiconv_size_t thickness;     /* Set to 1/20 for non-solid lines */
  psiconv_color color;          /* Border color */
} *psiconv_border;

/* Complete bullet information.
   The interaction of left and first line indentation and bullets is quite 
   complicated.

   BULLET       FIRST  BULLET      FIRST LINE             NEXT LINES

   None         = 0    -           Left+First             Left
                > 0    -           Left+First             Left
                < 0    -           Left+First             Left

   Indent Off   = 0    Left        Left(+Bullet)          Left
                > 0    Left        Left+First             Left
		< 0    Left+First  Left+First(+Bullet)    Left

   Indent On    = 0    Left        Left(+Bullet)          Left
                > 0    Left        Left+First             Left+First
		< 0    Left+First  Left                   Left
*/
typedef struct psiconv_bullet_s
{
  psiconv_bool_t on;           /* Whether the bullet is shown */
  psiconv_size_t font_size;    /* Bullet font size */
  psiconv_ucs2 character;      /* Bullet character */
  psiconv_bool_t indent;       /* Whether to indent (see above */
  psiconv_color color;         /* Bullet color */
  psiconv_font font;           /* Bullet font */
} *psiconv_bullet;

/* Complete single tab information */
typedef struct psiconv_tab_s
{
  psiconv_length_t location;      /* The indentation level */
  psiconv_tab_kind_t kind;        /* Tab kind */
} *psiconv_tab;

/* A list of tabs */
typedef psiconv_list psiconv_tab_list; /* of struct psiconv_tab_s */

/* Information about all tabs.
   Normal tabs start after the rightmost extra tab */
typedef struct psiconv_all_tabs_s
{
  psiconv_length_t normal;        /* Normal tab distance */
  psiconv_tab_list extras;        /* Additional defined tabs */
} *psiconv_all_tabs;

/* Character layout.
   This structure holds all layout information that can be applied on the
   character level (as opposed to layouts that only apply to whole 
   paragraphs). 
   Note that at all times, this structure holds the complete layout 
   information; we do not use incremental layouts, unlike the Psion
   file format itself. So if an italic text is also underlined, the 
   character_layout will have both set for that region. */
typedef struct psiconv_character_layout_s
{
  psiconv_color color;             /* Character color */
  psiconv_color back_color;        /* Background color */
  psiconv_size_t font_size;        /* Font size */
  psiconv_bool_t italic;           /* Use italics? */
  psiconv_bool_t bold;             /* Use bold? */
  psiconv_super_sub_t super_sub;   /* Use super/subscript? */
  psiconv_bool_t underline;        /* Underline? */
  psiconv_bool_t strikethrough;    /* Strike through? */
  psiconv_font font;               /* Character font */
} *psiconv_character_layout;

/* Paragraph layout.
   This structure holds all layout information that can be applied on the
   paragraph level (as opposed to layouts that also apply to single 
   characters). 
   Note that at all times, this structure holds the complete layout 
   information; we do not use incremental layouts, unlike the Psion 
   file format itself. 
   Linespacing is the amount of vertical space between lines. If
   linespacing_exact is set, this amount is used even if that would make
   text overlap; if it is not set, a greater distance is used if text would
   otherwise overlap.
   Several booleans determine where page breaks may be set. keep_together
   forbids page breaks in the middle of the paragraph; keep_with_next
   forbids page breaks between this and the next paragraph. on_next_page
   forces a pagebreak before the paragraph. no_widow_protection allows
   one single line of the paragraph on a page, and the rest on another page. 
   Sheet cell text normally does not wrap; wrap_to_fit_cell allows this. */
typedef struct psiconv_paragraph_layout_s
{
  psiconv_color back_color;           /* Background color */
  psiconv_length_t indent_left;       /* Left indentation (except first line) */
  psiconv_length_t indent_right;      /* Right indentation */
  psiconv_length_t indent_first;      /* First line left indentation */
  psiconv_justify_hor_t justify_hor;  /* Horizontal justification */
  psiconv_justify_ver_t justify_ver;  /* Vertical justification */
  psiconv_size_t linespacing;         /* The linespacing */
  psiconv_bool_t linespacing_exact;   /* Is linespacing exact or the minimum? */
  psiconv_size_t space_above;         /* Vertical space before the paragraph */
  psiconv_size_t space_below;         /* Vertical space after the paragraph */
  psiconv_bool_t keep_together;       /* Keep lines on one page? */
  psiconv_bool_t keep_with_next;      /* Disallow pagebreak after paragraph? */
  psiconv_bool_t on_next_page;        /* Force page break before paragraph? */
  psiconv_bool_t no_widow_protection; /* Undo widow protection? */
  psiconv_bool_t wrap_to_fit_cell;    /* Wrap sheet cell text? */
  psiconv_length_t border_distance;   /* Distance to borders */
  psiconv_bullet bullet;              /* Bullet information */
  psiconv_border left_border;         /* Left border information */
  psiconv_border right_border;        /* Right border information */
  psiconv_border top_border;          /* Top border information */
  psiconv_border bottom_border;       /* Bottom border information */
  psiconv_all_tabs tabs;              /* All tab information */
} *psiconv_paragraph_layout;

/* A Header Section.
   It contains the three UIDs and the checksum, and the type of file.
   As the type of file uniquely defines the UIDs, and as the UIDs determine
   the checksum, this is never included in a regular psiconv_file structure.
   It can be used to read the header section separately, though. */
typedef struct psiconv_header_section_s
{
  psiconv_u32 uid1;
  psiconv_u32 uid2;
  psiconv_u32 uid3;
  psiconv_u32 checksum;
  psiconv_file_type_t file;
} *psiconv_header_section;

/* A Section Table Section entry.
   Each entry has a UID and an offset.
   This is never included in a regular psiconv_file structure, as the 
   information is too low-level. It is used internally, though. */
typedef struct psiconv_section_table_entry_s
{
  psiconv_u32 id;           /* Section UID */
  psiconv_u32 offset;       /* Section offset within the file */
} *psiconv_section_table_entry;

/* A Section Table Section.
   A list of sections and their offsets.
   It is simply a list of Section Table Section Entries.
   This is never included in a regular psiconv_file structure, as the 
   information is too low-level. It is used internally, though. */
typedef psiconv_list psiconv_section_table_section; 
                                   /* Of struct psiconv_sectiontable_entry_s */

/* An Application ID Section.
   The type of file.
   Never included in a regular psiconv_file structure, because it is too
   low-level. You can always recover it if you know the file type. Used
   internally. 
   The name should probably be case-insensitive. */
typedef struct psiconv_application_id_section_s
{
  psiconv_u32 id;            /* File type UID */
  psiconv_string_t name;     /* File type name */
} *psiconv_application_id_section;

/* An Object Icon Section.
   The icon used for an embedded object. */
typedef struct psiconv_object_icon_section_s
{
  psiconv_length_t icon_width;      /* Icon width */
  psiconv_length_t icon_height;     /* Icon height */
  psiconv_string_t icon_name;       /* Icon name */
} *psiconv_object_icon_section;

/* An Object Display Section.
   How an embedded icon should be displayed. 
   The sizes are computed after cropping or resizing; if the object is shown
   as an icon, the icon sizes are used here. */
typedef struct psiconv_object_display_section_s
{
  psiconv_bool_t show_icon;         /* Show an icon or the whole file? */
  psiconv_length_t width;           /* Object display width */
  psiconv_length_t height;          /* Object display height */
} *psiconv_object_display_section;

/* An Embedded Object Section. 
   All data about an embedded object. 
   An object is another psiconv_file, which is embedded in the current
   file. Objects can also be embedded in each other, of course. */
typedef struct psiconv_embedded_object_section_s
{
  psiconv_object_icon_section icon;       /* Icon information */
  psiconv_object_display_section display; /* Display information */
  psiconv_file object;                    /* The object itself */
} *psiconv_embedded_object_section;

/* Inline character-level layout information.
   Information how some characters should be laid out. 
   Note that, though you can choose specific layouts for an object, this
   will probably not affect the object's rendering.
   Usually, object will be NULL, and the object_width and object_height 
   will be ignored. 
   The object sizes are the same as in the Object Display Section, so
   this information seems to be redundant. */
typedef struct psiconv_in_line_layout_s
{
  psiconv_character_layout layout;         /* Layout information */
  int length;                              /* Number of characters */
  psiconv_embedded_object_section object;  /* Embedded object or NULL */
  psiconv_length_t object_width;           /* Object display width */
  psiconv_length_t object_height;          /* Object display height */
} *psiconv_in_line_layout;

/* Inline character information for a whole line.
   A list of inline character information */
typedef psiconv_list psiconv_in_line_layouts; 
                                  /* of struct psiconv_in_line_layout_s */

/* What to replace where in text. Not yet implemented! 
   (not yet implemented!) */
typedef struct psiconv_replacement_s
{
  int offset;                       /* Offset in text */
  int cur_len;                      /* Length of text to replace */
  psiconv_replacement_type_t type;  /* Kind of replacement */
} *psiconv_replacement;

/* A list of replacements */
typedef psiconv_list psiconv_replacements; /* of struct psiconv_replacement_s */

/* A paragraph of text.
   Layout and actual paragraph text are combined here, even though
   they are seperated in the Psion file format.
   The base style is referred to, but the base_character and
   base_paragraph have all style settings already incorporated.
   The base style can be found using the psiconv_get_style function. 
   The in_lines are either an empty list, or they should apply to exactly
   the number of characters in this paragraph */
typedef struct psiconv_paragraph_s
{
  psiconv_string_t text;                     /* Paragraph text */
  psiconv_character_layout base_character;   /* Base character layout */
  psiconv_paragraph_layout base_paragraph;   /* Base paragraph layout */
  psiconv_s16 base_style;                    /* Paragraph style */
  psiconv_in_line_layouts in_lines;          /* In-paragraph layout */
  psiconv_replacements replacements;         /* Replacements like the date */
} *psiconv_paragraph;

/* A collection of text paragraphs */
typedef psiconv_list psiconv_text_and_layout; 
                                          /* Of struct psiconv_paragraph_s */

/* A TextEd Section.
   Text and simple layout, without styles.  */
typedef struct psiconv_texted_section_s
{
  psiconv_text_and_layout paragraphs; 
} *psiconv_texted_section;

/* A Page Header.
   All information about a page header or footer.
   An explicit base paragraph and character layout is found; this is used
   as a sort of base style, on which all further formatting is based */
typedef struct psiconv_page_header_s
{
  psiconv_bool_t on_first_page;                   /* Display on first page? */
  psiconv_paragraph_layout base_paragraph_layout; /* Base paragraph layout */
  psiconv_character_layout base_character_layout; /* Base character layout */
  psiconv_texted_section text;                    /* The actual text */
} *psiconv_page_header;

/* A Page Layout Section
   All information about the layout of a page.
   Margins, page size, the header, the footer and page numbers */
typedef struct psiconv_page_layout_section_s
{
  psiconv_u32 first_page_nr;         /* Page numbers start counting here */
  psiconv_length_t header_dist;      /* Distance of header to text */
  psiconv_length_t footer_dist;      /* Distance of footer to text */
  psiconv_length_t left_margin;      /* Left margin */
  psiconv_length_t right_margin;     /* Right margin */
  psiconv_length_t top_margin;       /* Top margin */
  psiconv_length_t bottom_margin;    /* Bottom margin */
  psiconv_length_t page_width;       /* Page width */
  psiconv_length_t page_height;      /* Page height */
  psiconv_page_header header;        /* Header information */
  psiconv_page_header footer;        /* Footer information */
  psiconv_bool_t landscape;          /* Landscape orientation? */
} * psiconv_page_layout_section;

/* A Word Status Section
   Settings of the Word program. 
   Several whitespace and related characters can be explicitely shown.
   Embedded pictures and graphs can be iconized or displayed full.
   Toolbars can be shown or hidden.
   Long lines can be wrapped, or you have to use scrolling.
   The cursor position is the character number of the text.
   Zoom level: 1000 is "normal" */
typedef struct psiconv_word_status_section_s
{
  psiconv_bool_t show_tabs;           /* Show tabs? */
  psiconv_bool_t show_spaces;         /* Show spaces? */
  psiconv_bool_t show_paragraph_ends; /* Show paragraph ends? */
  psiconv_bool_t show_line_breaks;    /* Show line breaks */
  psiconv_bool_t show_hard_minus;     /* Show hard dashes? */
  psiconv_bool_t show_hard_space;     /* Show hard spaces? */
  psiconv_bool_t show_full_pictures;  /* Show embedded pictures (or iconize)? */
  psiconv_bool_t show_full_graphs;    /* Show embedded graphs (or iconize)? */
  psiconv_bool_t show_top_toolbar;    /* Show top toolbar? */
  psiconv_bool_t show_side_toolbar;   /* Show side toolbar? */
  psiconv_bool_t fit_lines_to_screen; /* Wrap lines? */
  psiconv_u32 cursor_position;        /* Cursor position (character number) */
  psiconv_u32 display_size;           /* Zooming level */
} *psiconv_word_status_section;

/* A Word Style.
   All information about a single Style.
   A builtin style may not be changed in the Word program.
   Outline level is zero if unused. 
   The name may be NULL for the normal style! */
typedef struct psiconv_word_style_s
{
  psiconv_character_layout character; /* character-level layout */
  psiconv_paragraph_layout paragraph; /* paragraph-level layout */
  psiconv_ucs2 hotkey;                /* The hotkey */
  psiconv_string_t name;              /* Style name */
  psiconv_bool_t built_in;            /* Builtin style? */
  psiconv_u32 outline_level;          /* Outline level */
} *psiconv_word_style;

/* A list of Word Styles */
typedef psiconv_list psiconv_word_style_list; 
                                         /* Of struct psiconv_word_style_s */

/* A Word Styles Section
   All information about styles. 
   Note that the name of the normal style is NULL! */
typedef struct psiconv_word_styles_section_s
{
  psiconv_word_style normal;      /* The normal (unspecified) style */
  psiconv_word_style_list styles; /* All other defined styles */
} *psiconv_word_styles_section;

/* A Word File
   All information about a Word File.
   Note that a section can be NULL if it is not present. */
typedef struct psiconv_word_f_s
{
  psiconv_page_layout_section page_sec;    /* Page layout */
  psiconv_text_and_layout paragraphs;      /* Text and text layout */
  psiconv_word_status_section status_sec;  /* Internal Word program settings */
  psiconv_word_styles_section styles_sec;  /* Styles */
} *psiconv_word_f;

/* A TextEd File
   All information about a TextEd File.
   Note that a section can be NULL if it is not present. */
typedef struct psiconv_texted_f_s
{
  psiconv_page_layout_section page_sec;    /* Page layout */
  psiconv_texted_section texted_sec;       /* Text and text layout */
} *psiconv_texted_f;

/* A Jumptable Section.
   A simple list of offsets.
   This is never included in a regular psiconv_file structure, as the 
   information is too low-level. It is used internally, though. */
typedef psiconv_list psiconv_jumptable_section; /* of psiconv_u32 */

/* A Paint Data Section
   A collection of pixels.
   Normalized values [0..1] for each color component.
   Origin is (x,y)=(0,0), to get pixel at (X,Y) use index [Y*xsize+X] */
typedef struct psiconv_paint_data_section_s
{
  psiconv_u32 xsize;          /* Number of pixels in a row */
  psiconv_u32 ysize;          /* Number of pixels in a column */
  psiconv_length_t pic_xsize; /* 0 if not specified */
  psiconv_length_t pic_ysize; /* 0 if not specified */
  float *red;                 
  float *green;
  float *blue;
} *psiconv_paint_data_section;

/* A collection of Paint Data Sections */
typedef psiconv_list psiconv_pictures; 
            /* of struct psiconv_paint_data_section_s */

/* A MBM file 
   All information about a MBM file
   MBM files contain one or more pictures. */
typedef struct psiconv_mbm_f_s 
{
  psiconv_pictures sections;
} *psiconv_mbm_f;

/* Read the Psiconv file format documentation for a complete discription.
   Basic idea: a picture has a certain display size. Within it, the pixel
   data begins at a certain offset. Around it, there is an empty form.
   The first eight values are before magnification and cuts.
   Cuts are always <= 1.0; a cut of 0.0 cuts nothing away, a cut of 1.0
   cuts everything away. */
typedef struct psiconv_sketch_section_s
{
  psiconv_u16 displayed_xsize;
  psiconv_u16 displayed_ysize;
  psiconv_u16 picture_data_x_offset;
  psiconv_u16 picture_data_y_offset;
  psiconv_u16 form_xsize; 
  psiconv_u16 form_ysize;
  psiconv_u16 displayed_size_x_offset;
  psiconv_u16 displayed_size_y_offset;
  float magnification_x; /* computed relative to first eight values */
  float magnification_y; /* computed relative to first eight values */
  float cut_left;        /* computed relative to first eight values */
  float cut_right;       /* computed relative to first eight values */
  float cut_top;         /* computed relative to first eight values */
  float cut_bottom;      /* computed relative to first eight values */
  psiconv_paint_data_section picture;
} *psiconv_sketch_section;

typedef struct psiconv_sketch_f_s 
{
  psiconv_sketch_section sketch_sec;
} *psiconv_sketch_f;

typedef struct psiconv_clipart_section_s 
{
  /* Perhaps later on some currently unknown stuff. */
  psiconv_paint_data_section picture;
} * psiconv_clipart_section;

typedef psiconv_list psiconv_cliparts; /* of struct psiconv_clipart_section_s */

typedef struct psiconv_clipart_f_s 
{
  psiconv_cliparts sections;
} *psiconv_clipart_f;

typedef struct psiconv_sheet_ref_s
{
  psiconv_s16 offset;
  psiconv_bool_t absolute;
} psiconv_sheet_ref_t;

typedef struct psiconv_sheet_cell_reference_s
{
  psiconv_sheet_ref_t row;
  psiconv_sheet_ref_t column;
} psiconv_sheet_cell_reference_t;

typedef struct psiconv_sheet_cell_block_s
{
  psiconv_sheet_cell_reference_t first;
  psiconv_sheet_cell_reference_t last;
} psiconv_sheet_cell_block_t;

typedef enum psiconv_cell_type
{
  psiconv_cell_blank,
  psiconv_cell_int,
  psiconv_cell_bool,
  psiconv_cell_error,
  psiconv_cell_float,
  psiconv_cell_string
} psiconv_cell_type_t;

typedef enum psiconv_sheet_errorcode
{
  psiconv_sheet_error_none,
  psiconv_sheet_error_null,
  psiconv_sheet_error_divzero,
  psiconv_sheet_error_value,
  psiconv_sheet_error_reference,
  psiconv_sheet_error_name,
  psiconv_sheet_error_number,
  psiconv_sheet_error_notavail
} psiconv_sheet_errorcode_t;

typedef enum psiconv_sheet_numberformat_code
{
  psiconv_numberformat_general,
  psiconv_numberformat_fixeddecimal,
  psiconv_numberformat_scientific,
  psiconv_numberformat_currency,
  psiconv_numberformat_percent,
  psiconv_numberformat_triads,
  psiconv_numberformat_boolean,
  psiconv_numberformat_text,
  psiconv_numberformat_date_dmm,
  psiconv_numberformat_date_mmd,
  psiconv_numberformat_date_ddmmyy,
  psiconv_numberformat_date_mmddyy,
  psiconv_numberformat_date_yymmdd,
  psiconv_numberformat_date_dmmm,
  psiconv_numberformat_date_dmmmyy,
  psiconv_numberformat_date_ddmmmyy,
  psiconv_numberformat_date_mmm,
  psiconv_numberformat_date_monthname,
  psiconv_numberformat_date_mmmyy,
  psiconv_numberformat_date_monthnameyy,
  psiconv_numberformat_date_monthnamedyyyy,
  psiconv_numberformat_datetime_ddmmyyyyhhii,
  psiconv_numberformat_datetime_ddmmyyyyHHii,
  psiconv_numberformat_datetime_mmddyyyyhhii,
  psiconv_numberformat_datetime_mmddyyyyHHii,
  psiconv_numberformat_datetime_yyyymmddhhii,
  psiconv_numberformat_datetime_yyyymmddHHii,
  psiconv_numberformat_time_hhii,
  psiconv_numberformat_time_hhiiss,
  psiconv_numberformat_time_HHii,
  psiconv_numberformat_time_HHiiss
} psiconv_sheet_numberformat_code_t;

typedef struct psiconv_sheet_numberformat_s
{
  psiconv_sheet_numberformat_code_t code;
  psiconv_u8 decimal;
} *psiconv_sheet_numberformat;

typedef struct psiconv_sheet_cell_layout_s
{
  psiconv_character_layout character;
  psiconv_paragraph_layout paragraph;
  psiconv_sheet_numberformat numberformat;  
} * psiconv_sheet_cell_layout;

typedef struct psiconv_sheet_cell_s
{
  psiconv_u16 column;
  psiconv_u16 row;
  psiconv_cell_type_t type;
  union {
    psiconv_u32 dat_int;
    double dat_float;
    psiconv_string_t dat_string;
    psiconv_bool_t dat_bool;
    psiconv_sheet_errorcode_t dat_error;
  } data;
  psiconv_sheet_cell_layout layout;
  psiconv_bool_t calculated;
  psiconv_u32 ref_formula;
} *psiconv_sheet_cell;

typedef psiconv_list psiconv_sheet_cell_list; 
                                         /* Of struct psiconv_sheet_cell_s */

typedef struct psiconv_sheet_status_section_s
{
  psiconv_bool_t show_graph;
  psiconv_u32 cursor_row;
  psiconv_u32 cursor_column;
  psiconv_bool_t show_top_sheet_toolbar;
  psiconv_bool_t show_side_sheet_toolbar;
  psiconv_bool_t show_top_graph_toolbar;
  psiconv_bool_t show_side_graph_toolbar;
  psiconv_u32 sheet_display_size;
  psiconv_u32 graph_display_size;
  psiconv_triple_t show_horizontal_scrollbar;
  psiconv_triple_t show_vertical_scrollbar;
} *psiconv_sheet_status_section;

typedef enum psiconv_formula_type
{
  psiconv_formula_unknown,
  psiconv_formula_op_lt,
  psiconv_formula_op_le,
  psiconv_formula_op_gt,
  psiconv_formula_op_ge,
  psiconv_formula_op_ne,
  psiconv_formula_op_eq,
  psiconv_formula_op_add,
  psiconv_formula_op_sub,
  psiconv_formula_op_mul,
  psiconv_formula_op_div,
  psiconv_formula_op_pow,
  psiconv_formula_op_pos,
  psiconv_formula_op_neg,
  psiconv_formula_op_not,
  psiconv_formula_op_and,
  psiconv_formula_op_or,
  psiconv_formula_op_con,
  psiconv_formula_op_bra,
  psiconv_formula_mark_eof,
  psiconv_formula_dat_float,
  psiconv_formula_dat_int,
  psiconv_formula_dat_var,
  psiconv_formula_dat_string,
  psiconv_formula_dat_cellref,
  psiconv_formula_dat_cellblock,
  psiconv_formula_dat_vcellblock,
  psiconv_formula_mark_opsep,
  psiconv_formula_mark_opend,
  psiconv_formula_fun_false,
  psiconv_formula_fun_if,
  psiconv_formula_fun_true,
  psiconv_formula_fun_cell,
  psiconv_formula_fun_errortype,
  psiconv_formula_fun_isblank,
  psiconv_formula_fun_iserr,
  psiconv_formula_fun_iserror,
  psiconv_formula_fun_islogical,
  psiconv_formula_fun_isna,
  psiconv_formula_fun_isnontext,
  psiconv_formula_fun_isnumber,
  psiconv_formula_fun_istext,
  psiconv_formula_fun_n,
  psiconv_formula_fun_type,
  psiconv_formula_fun_address,
  psiconv_formula_fun_column,
  psiconv_formula_fun_columns,
  psiconv_formula_fun_hlookup,
  psiconv_formula_fun_index,
  psiconv_formula_fun_indirect,
  psiconv_formula_fun_lookup,
  psiconv_formula_fun_offset,
  psiconv_formula_fun_row,
  psiconv_formula_fun_rows,
  psiconv_formula_fun_vlookup,
  psiconv_formula_fun_char,
  psiconv_formula_fun_code,
  psiconv_formula_fun_exact,
  psiconv_formula_fun_find,
  psiconv_formula_fun_left,
  psiconv_formula_fun_len,
  psiconv_formula_fun_lower,
  psiconv_formula_fun_mid,
  psiconv_formula_fun_proper,
  psiconv_formula_fun_replace,
  psiconv_formula_fun_rept,
  psiconv_formula_fun_right,
  psiconv_formula_fun_string,
  psiconv_formula_fun_t,
  psiconv_formula_fun_trim,
  psiconv_formula_fun_upper,
  psiconv_formula_fun_value,
  psiconv_formula_fun_date,
  psiconv_formula_fun_datevalue,
  psiconv_formula_fun_day,
  psiconv_formula_fun_hour,
  psiconv_formula_fun_minute,
  psiconv_formula_fun_month,
  psiconv_formula_fun_now,
  psiconv_formula_fun_second,
  psiconv_formula_fun_today,
  psiconv_formula_fun_time,
  psiconv_formula_fun_timevalue,
  psiconv_formula_fun_year,
  psiconv_formula_fun_abs,
  psiconv_formula_fun_acos,
  psiconv_formula_fun_asin,
  psiconv_formula_fun_atan,
  psiconv_formula_fun_atan2,
  psiconv_formula_fun_cos,
  psiconv_formula_fun_degrees,
  psiconv_formula_fun_exp,
  psiconv_formula_fun_fact,
  psiconv_formula_fun_int,
  psiconv_formula_fun_ln,
  psiconv_formula_fun_log10,
  psiconv_formula_fun_mod,
  psiconv_formula_fun_pi,
  psiconv_formula_fun_radians,
  psiconv_formula_fun_rand,
  psiconv_formula_fun_round,
  psiconv_formula_fun_sign,
  psiconv_formula_fun_sin,
  psiconv_formula_fun_sqrt,
  psiconv_formula_fun_sumproduct,
  psiconv_formula_fun_tan,
  psiconv_formula_fun_trunc,
  psiconv_formula_fun_cterm,
  psiconv_formula_fun_ddb,
  psiconv_formula_fun_fv,
  psiconv_formula_fun_irr,
  psiconv_formula_fun_npv,
  psiconv_formula_fun_pmt,
  psiconv_formula_fun_pv,
  psiconv_formula_fun_rate,
  psiconv_formula_fun_sln,
  psiconv_formula_fun_syd,
  psiconv_formula_fun_term,
  psiconv_formula_fun_combin,
  psiconv_formula_fun_permut,
  psiconv_formula_vfn_average,
  psiconv_formula_vfn_choose,
  psiconv_formula_vfn_count,
  psiconv_formula_vfn_counta,
  psiconv_formula_vfn_countblank,
  psiconv_formula_vfn_max,
  psiconv_formula_vfn_min,
  psiconv_formula_vfn_product,
  psiconv_formula_vfn_stdevp,
  psiconv_formula_vfn_stdev,
  psiconv_formula_vfn_sum,
  psiconv_formula_vfn_sumsq,
  psiconv_formula_vfn_varp,
  psiconv_formula_vfn_var
} psiconv_formula_type_t;

typedef psiconv_list psiconv_formula_list; /* Of struct psiconv_formula_s */

typedef struct psiconv_formula_s
{
  psiconv_formula_type_t type;
  union {
    psiconv_u32 dat_int;
    double dat_float;
    psiconv_string_t dat_string;
    psiconv_sheet_cell_reference_t dat_cellref;
    psiconv_sheet_cell_block_t dat_cellblock;
    psiconv_formula_list fun_operands;
    psiconv_u32 dat_variable;
  } data;
} *psiconv_formula;

typedef struct psiconv_sheet_line_s
{
  psiconv_u32 position;
  psiconv_sheet_cell_layout layout;
} *psiconv_sheet_line;

typedef psiconv_list psiconv_sheet_line_list; 
                     /* Of struct psiconv_sheet_line_s */

typedef struct psiconv_sheet_grid_size_s
{
  psiconv_u32 line_number;
  psiconv_length_t size;
} *psiconv_sheet_grid_size;

typedef psiconv_list psiconv_sheet_grid_size_list; 
                    /* Of struct psiconv_sheet_grid_size_s */

typedef psiconv_list psiconv_sheet_grid_break_list; /* of psiconv_u32 */

typedef struct psiconv_sheet_grid_section_s
{
  psiconv_bool_t show_column_titles;
  psiconv_bool_t show_row_titles;
  psiconv_bool_t show_vertical_grid;
  psiconv_bool_t show_horizontal_grid;
  psiconv_bool_t freeze_rows;
  psiconv_bool_t freeze_columns;
  psiconv_u32 frozen_rows;
  psiconv_u32 frozen_columns;
  psiconv_u32 first_unfrozen_row_displayed;
  psiconv_u32 first_unfrozen_column_displayed;
  psiconv_bool_t show_page_breaks;
  psiconv_u32 first_row;
  psiconv_u32 first_column;
  psiconv_u32 last_row;
  psiconv_u32 last_column;
  psiconv_length_t default_row_height;
  psiconv_length_t default_column_width;
  psiconv_sheet_grid_size_list row_heights;
  psiconv_sheet_grid_size_list column_heights;
  psiconv_sheet_grid_break_list row_page_breaks;
  psiconv_sheet_grid_break_list column_page_breaks;
} *psiconv_sheet_grid_section;

typedef struct psiconv_sheet_worksheet_s
{
  psiconv_sheet_cell_layout default_layout;
  psiconv_sheet_cell_list cells;
  psiconv_bool_t show_zeros;
  psiconv_sheet_line_list row_default_layouts;
  psiconv_sheet_line_list col_default_layouts;
  psiconv_sheet_grid_section grid;
} *psiconv_sheet_worksheet;

typedef psiconv_list psiconv_sheet_worksheet_list; 
        /* of struct psiconv_sheet_worksheet_s */

typedef enum psiconv_variable_type
{
  psiconv_var_int,
  psiconv_var_float,
  psiconv_var_string,
  psiconv_var_cellref,
  psiconv_var_cellblock
} psiconv_variable_type_t;

typedef struct psiconv_sheet_variable_s
{
  psiconv_u32 number;
  psiconv_string_t name;
  psiconv_variable_type_t type;
  union {
    psiconv_s32 dat_int;
    double dat_float;
    psiconv_string_t dat_string;
    psiconv_sheet_cell_reference_t dat_cellref;
    psiconv_sheet_cell_block_t dat_cellblock;
  } data;
} *psiconv_sheet_variable;
  
typedef psiconv_list psiconv_sheet_variable_list; 
        /* of struct psiconv_sheet_variable_s */

typedef struct psiconv_sheet_name_section_s
{
  psiconv_string_t name;
} *psiconv_sheet_name_section;

typedef struct psiconv_sheet_info_section_s
{
  psiconv_bool_t auto_recalc;
} *psiconv_sheet_info_section;

typedef struct psiconv_sheet_workbook_section_s
{
  psiconv_formula_list formulas;
  psiconv_sheet_worksheet_list worksheets;
  psiconv_sheet_variable_list variables;
  psiconv_sheet_info_section info;
  psiconv_sheet_name_section name;
} *psiconv_sheet_workbook_section;

typedef struct psiconv_sheet_f_s
{
  psiconv_page_layout_section page_sec;
  psiconv_sheet_status_section status_sec;
  psiconv_sheet_workbook_section workbook_sec;
} *psiconv_sheet_f;

/* NB: psiconv_file is already defined above */
struct psiconv_file_s 
{
  psiconv_file_type_t type;
  void *file;
};


/* UID1 */
#define PSICONV_ID_PSION5 0x10000037
#define PSICONV_ID_CLIPART 0x10000041
/* UID2 */
#define PSICONV_ID_DATA_FILE 0x1000006D
#define PSICONV_ID_MBM_FILE 0x10000042
/* UID3 */
#define PSICONV_ID_WORD 0x1000007F
#define PSICONV_ID_TEXTED 0x10000085
#define PSICONV_ID_SKETCH 0x1000007D
#define PSICONV_ID_SHEET 0x10000088

/* Section table ids */
#define PSICONV_ID_WORD_STATUS_SECTION 0x10000243
#define PSICONV_ID_APPL_ID_SECTION 0x10000089
#define PSICONV_ID_TEXT_SECTION 0x10000106
#define PSICONV_ID_LAYOUT_SECTION 0x10000143
#define PSICONV_ID_WORD_STYLES_SECTION 0x10000104
#define PSICONV_ID_PAGE_LAYOUT_SECTION 0x10000105
#define PSICONV_ID_PASSWORD_SECTION 0x100000CD
#define PSICONV_ID_SKETCH_SECTION 0x1000007D
#define PSICONV_ID_SHEET_STATUS_SECTION 0x1000011F
#define PSICONV_ID_SHEET_WORKBOOK_SECTION 0x1000011D
#define PSICONV_ID_SHEET_GRAPH_SECTION 0x10000121

/* Other ids */
#define PSICONV_ID_PAGE_DIMENSIONS1 0x100000fd
#define PSICONV_ID_PAGE_DIMENSIONS2 0x1000010e
#define PSICONV_ID_TEXTED_BODY 0x1000005c
#define PSICONV_ID_TEXTED_REPLACEMENT 0x10000063
#define PSICONV_ID_TEXTED_UNKNOWN 0x10000065
#define PSICONV_ID_TEXTED_LAYOUT 0x10000066
#define PSICONV_ID_TEXTED_TEXT 0x10000064
#define PSICONV_ID_STYLE_REMOVABLE 0x1000004F
#define PSICONV_ID_STYLE_BUILT_IN 0x1000004C
#define PSICONV_ID_CLIPART_ITEM 0x10000040
#define PSICONV_ID_OBJECT 0x10000051
#define PSICONV_ID_OBJECT_DISPLAY_SECTION 0x10000146
#define PSICONV_ID_OBJECT_ICON_SECTION 0x1000012A
#define PSICONV_ID_OBJECT_SECTION_TABLE_SECTION 0x10000144


/* Return a clean layout_status. You can modify it at will. Returns NULL
   if there is not enough memory. */
extern psiconv_character_layout psiconv_basic_character_layout(void);

/* Return a clean layout_status. You can modify it at will. Returns NULL
   if there is not enough memory. */
extern psiconv_paragraph_layout psiconv_basic_paragraph_layout(void);

/* Clone a layout_status: the new copy is completely independent of the
   original one. Returns NULL if there is not enough memory. */
extern psiconv_paragraph_layout psiconv_clone_paragraph_layout
                                       (psiconv_paragraph_layout ls);

extern psiconv_character_layout psiconv_clone_character_layout
                                (psiconv_character_layout ls);

/* Get a numbered style. Returns NULL if the style is unknown. */
extern psiconv_word_style psiconv_get_style (psiconv_word_styles_section ss, int nr);

/* Return the number corresponding to the stylename. Returns 0 on success,
   an error code on failure. */
extern int psiconv_find_style(const psiconv_word_styles_section ss,
                              const psiconv_ucs2 *name,int *nr);

/* Get a numbered formula. Returns NULL if the style is unknown. */
extern psiconv_formula psiconv_get_formula (psiconv_formula_list ss, int nr);

/* Return the default layout */
extern psiconv_sheet_cell_layout psiconv_get_default_layout
                                       (psiconv_sheet_line_list row_defaults,
                                        psiconv_sheet_line_list col_defaults,
                                        psiconv_sheet_cell_layout cell_default,
                                        int row,int col);

extern void psiconv_free_color(psiconv_color color);
extern void psiconv_free_border(psiconv_border border);
extern void psiconv_free_bullet(psiconv_bullet bullet);
extern void psiconv_free_font(psiconv_font font);
extern void psiconv_free_tab(psiconv_tab tab);
extern void psiconv_free_tabs(psiconv_all_tabs tabs);
extern void psiconv_free_paragraph_layout(psiconv_paragraph_layout layout);
extern void psiconv_free_character_layout(psiconv_character_layout layout);
extern void psiconv_free_word_style(psiconv_word_style style);
extern void psiconv_free_word_style_list(psiconv_word_style_list style_list);
extern void psiconv_free_word_styles_section
                                      (psiconv_word_styles_section styles);
extern void psiconv_free_formula(psiconv_formula formula);
extern void psiconv_free_formula_list(psiconv_formula_list list);
extern void psiconv_free_sheet_status_section
                                      (psiconv_sheet_status_section section);
extern void psiconv_free_sheet_cell_layout(psiconv_sheet_cell_layout layout);
extern void psiconv_free_sheet_grid_break_list
                                        (psiconv_sheet_grid_break_list list);
extern void psiconv_free_sheet_grid_size(psiconv_sheet_grid_size s);
extern void psiconv_free_sheet_grid_size_list
                                        (psiconv_sheet_grid_size_list list);
extern void psiconv_free_sheet_grid_section(psiconv_sheet_grid_section sec);
extern void psiconv_free_sheet_worksheet(psiconv_sheet_worksheet sheet);
extern void psiconv_free_sheet_worksheet_list
                                         (psiconv_sheet_worksheet_list list);

extern void psiconv_free_sheet_f(psiconv_sheet_f file);
extern void psiconv_free_sheet_cell(psiconv_sheet_cell cell);
extern void psiconv_free_sheet_cell_list(psiconv_sheet_cell_list list);
extern void psiconv_free_sheet_numberformat
                                    (psiconv_sheet_numberformat numberformat);
extern void psiconv_free_sheet_line_list(psiconv_sheet_line_list list);
extern void psiconv_free_sheet_line(psiconv_sheet_line line);
extern void psiconv_free_sheet_name_section(psiconv_sheet_name_section section);
extern void psiconv_free_sheet_variable(psiconv_sheet_variable list);
extern void psiconv_free_sheet_variable_list(psiconv_sheet_variable_list list);
extern void psiconv_free_sheet_info_section(psiconv_sheet_info_section section);
extern void psiconv_free_sheet_workbook_section
                                      (psiconv_sheet_workbook_section section);
extern void psiconv_free_header_section(psiconv_header_section header);
extern void psiconv_free_section_table_entry(psiconv_section_table_entry entry);
extern void psiconv_free_section_table_section
                                   (psiconv_section_table_section section);
extern void psiconv_free_application_id_section
                                   (psiconv_application_id_section section);
extern void psiconv_free_object_display_section
                                   (psiconv_object_display_section section);
extern void psiconv_free_object_icon_section
                                   (psiconv_object_icon_section section);
extern void psiconv_free_embedded_object_section
                                   (psiconv_embedded_object_section object);
extern void psiconv_free_in_line_layout(psiconv_in_line_layout layout);
extern void psiconv_free_in_line_layouts(psiconv_in_line_layouts layouts);
extern void psiconv_free_replacement(psiconv_replacement replacement);
extern void psiconv_free_replacements(psiconv_replacements replacements);
extern void psiconv_free_paragraph(psiconv_paragraph paragraph);
extern void psiconv_free_text_and_layout(psiconv_text_and_layout text);
extern void psiconv_free_texted_section(psiconv_texted_section section);
extern void psiconv_free_page_header(psiconv_page_header header);
extern void psiconv_free_page_layout_section
                                      (psiconv_page_layout_section section);
extern void psiconv_free_word_status_section
                                      (psiconv_word_status_section section);
extern void psiconv_free_word_f(psiconv_word_f file);
extern void psiconv_free_texted_f(psiconv_texted_f file);
extern void psiconv_free_paint_data_section(psiconv_paint_data_section section);
extern void psiconv_free_pictures(psiconv_pictures section);
extern void psiconv_free_jumptable_section
                                       (psiconv_jumptable_section section);
extern void psiconv_free_mbm_f(psiconv_mbm_f file);
extern void psiconv_free_sketch_section(psiconv_sketch_section sec);
extern void psiconv_free_sketch_f(psiconv_sketch_f file);
extern void psiconv_free_clipart_section(psiconv_clipart_section section);
extern void psiconv_free_cliparts(psiconv_cliparts section);
extern void psiconv_free_clipart_f(psiconv_clipart_f file);

extern void psiconv_free_file(psiconv_file file);

extern int psiconv_compare_color(const psiconv_color value1,
                                 const psiconv_color value2);
extern int psiconv_compare_font(const psiconv_font value1,
                                const psiconv_font value2);
extern int psiconv_compare_border(const psiconv_border value1,
                                  const psiconv_border value2);
extern int psiconv_compare_bullet(const psiconv_bullet value1,
                                  const psiconv_bullet value2);
extern int psiconv_compare_tab(const psiconv_tab value1, 
                               const psiconv_tab value2);
extern int psiconv_compare_all_tabs(const psiconv_all_tabs value1,
                                    const psiconv_all_tabs value2);
extern int psiconv_compare_paragraph_layout
                               (const psiconv_paragraph_layout value1,
                                const psiconv_paragraph_layout value2);

extern int psiconv_compare_character_layout
                               (const psiconv_character_layout value1,
                                const psiconv_character_layout value2);

/* Get a newly allocated file with sensible defaults, ready to generate. */
extern psiconv_file psiconv_empty_file(psiconv_file_type_t type);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* def PSICONV_DATA_H */
