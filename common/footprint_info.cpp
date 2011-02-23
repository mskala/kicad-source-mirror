/**
 * @file footprint_info.cpp
 */


/*
 * Functions to read footprint libraries and fill m_footprints by available footprints names
 * and their documentation (comments and keywords)
 */
#include "fctsys.h"
#include "wxstruct.h"
#include "common.h"
#include "kicad_string.h"
#include "macros.h"
#include "appl_wxstruct.h"
#include "pcbstruct.h"
#include "pcbcommon.h"
#include "richio.h"
#include "filter_reader.h"
#include "footprint_info.h"


/* Read the list of libraries (*.mod files)
 * for each module are stored
 *      the module name
 *      documentation string
 *      associated keywords
 *      lib name
 * Module description format:
 *   $MODULE c64acmd                    First line of module description
 *   Li c64acmd DIN connector           Library reference
 *   Cd Europe 96 AC male vertical      documentation string
 *   Kw PAD_CONN DIN                    associated keywords
 *   ...... other data (pads, outlines ..)
 *   $Endmodule
 */
bool FOOTPRINT_LIST::ReadFootprintFiles( wxArrayString & aFootprintsLibNames )
{
    FILE*       file;
    wxFileName  filename;
    wxString    libname;

    // Clear data before reading files
    m_filesNotFound.Empty();
    m_filesInvalid.Empty();
    m_List.clear();

    /* Parse Libraries Listed */
    for( unsigned ii = 0; ii < aFootprintsLibNames.GetCount(); ii++ )
    {
        filename = aFootprintsLibNames[ii];
        filename.SetExt( ModuleFileExtension );

        libname = wxGetApp().FindLibraryPath( filename );

        if( libname.IsEmpty() )
        {
            m_filesNotFound << filename.GetFullName() << wxT("\n");
            continue;
        }

        /* Open library file */
        file = wxFopen( libname, wxT( "rt" ) );

        if( file == NULL )
        {
            m_filesInvalid << libname <<  _(" (file cannot be opened)") << wxT("\n");
            continue;
        }

        FILE_LINE_READER    fileReader( file, libname );
        FILTER_READER       reader( fileReader );

        /* Read header. */
        reader.ReadLine();
        char * line = reader.Line();
        StrPurge( line );

        if( strnicmp( line, ENTETE_LIBRAIRIE, L_ENTETE_LIB ) != 0 )
        {
            wxString msg;
            msg.Printf( _( "<%s> is not a valid Kicad PCB footprint library." ),
                        GetChars( libname ) );
            m_filesInvalid << msg << wxT("\n");
            continue;
        }

        // Read library
        bool end = false;
        while( !end && reader.ReadLine() )
        {
            line = reader.Line();
            StrPurge( line );
            if( strnicmp( line, "$EndLIBRARY", 11 ) == 0 )
            {
                end = true;
                break;
            }
            if( strnicmp( line, "$MODULE", 7 ) == 0 )
            {

                line += 7;
                FOOTPRINT_INFO*  ItemLib = new FOOTPRINT_INFO();
                ItemLib->m_Module = CONV_FROM_UTF8( StrPurge( line ) );
                ItemLib->m_LibName = libname;
                AddItem( ItemLib );

                while( reader.ReadLine() )
                {
                    line = reader.Line();
                    StrPurge( line );
                    if( strnicmp( line, "$EndMODULE", 10 ) == 0 )
                        break;

                    int id = ((line[0] & 0xFF) << 8) + (line[1] & 0xFF);
                    switch( id )
                    {
                    /* KeyWords */
                    case (('K'<<8) + 'w'):
                        ItemLib->m_KeyWord = CONV_FROM_UTF8( StrPurge( line + 3 ) );
                    break;

                    /* Doc */
                    case (('C'<<8) + 'd'):
                        ItemLib->m_Doc = CONV_FROM_UTF8( StrPurge( line + 3 ) );
                    break;
                    }
                }
            }
        }

        if( !end )
        {
            m_filesInvalid << libname << _(" (Unexpected end of file)") << wxT("\n");
        }
    }

    m_List.sort();

    return true;
}