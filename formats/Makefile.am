GENERATE = psion/Application_ID_Section.psi \
           psion/ASCII_Codes.psi \
           psion/Basic_Elements.psi \
           psion/Basic_Structures.psi \
           psion/Clip_Art_File.psi \
           psion/Embedded_Object_Section.psi \
           psion/File_Structure.psi \
           psion/Fonts.psi \
           psion/Header_Section.psi \
           psion/Identifiers.psi \
           psion/Index.psi \
           psion/Introduction.psi \
           psion/Layout_Codes.psi \
           psion/MBM_File.psi \
           psion/Page_Layout_Section.psi \
           psion/Paint_Data_Section.psi \
           psion/Password_Section.psi \
           psion/Record_File.psi \
           psion/Record_Section.psi \
           psion/Section_Table_Offset_Section.psi \
           psion/Section_Table_Section.psi \
           psion/Sheet_Basic_Structures.psi \
           psion/Sheet_Cell_List.psi \
	   psion/Sheet_File.psi \
           psion/Sheet_Formula_List.psi \
           psion/Sheet_Graph_Description.psi \
           psion/Sheet_Graph_List_Section.psi \
           psion/Sheet_Graph_Region.psi \
           psion/Sheet_Graph_Section.psi \
           psion/Sheet_Grid_Section.psi \
           psion/Sheet_Info_Section.psi \
           psion/Sheet_Line_Section.psi \
           psion/Sheet_Name_Section.psi \
           psion/Sheet_Status_Section.psi \
           psion/Sheet_Variable_List.psi \
           psion/Sheet_Workbook_Section.psi \
           psion/Sheet_Worksheet.psi \
           psion/Sheet_Worksheet_List.psi \
           psion/Sketch_File.psi \
           psion/Sketch_Section.psi \
           psion/Substitutions.psi \
           psion/TextEd_File.psi \
           psion/TextEd_Section.psi \
           psion/Text_Layout_Section.psi \
           psion/Text_Section.psi \
           psion/Userdic_File.psi \
           psion/Word_File.psi \
           psion/Word_Status_Section.psi \
           psion/Word_Styles_Section.psi \
           psion/World_Data_File.psi

EXTRA_DIST=$(GENERATE) generate_ascii.sh generate_xhtml.sh generate_html4.sh \
           html4_links.sh xhtml_links.sh index_html.sh psiconv.conf

nobase_pkgdata_DATA=$(XHTMLDOCFILES) $(HTML4DOCFILES) $(ASCIIDOCFILES) \
	            $(PSIONFILES)

all-local: .touch-xhtml .touch-html4 .touch-ascii 

PSIONFILES=$(GENERATE)

.touch-xhtml: $(GENERATE) 
if XHTMLDOCS
	rm -rf xhtml
	mkdir xhtml
	./generate_xhtml.sh .. xhtml $(GENERATE) 
	touch .touch-xhtml

XHTMLDOCFILES=$(patsubst %.psi,%.html,$(patsubst psion/%,xhtml/%,$(GENERATE)))
$(XHTMLDOCFILES): .touch-xhtml
endif

.touch-html4: $(GENERATE) 
if HTML4DOCS
	rm -rf html4
	mkdir html4
	./generate_html4.sh .. html4 $(GENERATE)
	touch .touch-html4

HTML4DOCFILES=$(patsubst %.psi,%.html,$(patsubst psion/%,html4/%,$(GENERATE)))
$(HTML4DOCFILES): .touch-html4
endif

.touch-ascii: $(GENERATE) 
if ASCIIDOCS
	rm -rf ascii
	mkdir ascii
	./generate_ascii.sh .. ascii $(GENERATE) 
	touch .touch-ascii

ASCIIDOCFILES=$(patsubst %.psi,%.ascii,$(patsubst psion/%,ascii/%,$(GENERATE)))
$(ASCIIDOCFILES): .touch-ascii
endif

clean-local:
	rm -rf xhtml html4 ascii .touch-*
