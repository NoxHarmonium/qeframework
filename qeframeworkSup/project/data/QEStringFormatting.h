/*  QEStringFormatting.h
 *
 *  This file is part of the EPICS QT Framework, initially developed at the
 *  Australian Synchrotron.
 *
 *  Copyright (c) 2009-2021 Australian Synchrotron
 *
 *  The EPICS QT Framework is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  The EPICS QT Framework is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with the EPICS QT Framework.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Author:
 *    Andrew Rhyder
 *  Contact details:
 *    andrew.rhyder@synchrotron.org.au
 */

// Formats the string for QEString data.

#ifndef QE_STRING_FORMATTING_H
#define QE_STRING_FORMATTING_H

#include <QVariant>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QDataStream>
#include <QTextStream>

#include <QELocalEnumeration.h>
#include <QEFrameworkLibraryGlobal.h>


class QE_FRAMEWORK_LIBRARY_SHARED_EXPORT QEStringFormatting {
public:

    /// \enum formats
    /// Formatting options
    enum formats { FORMAT_DEFAULT,              ///< Format according to the EPICS database record type
                   FORMAT_FLOATING,             ///< Format as a floating point number
                   FORMAT_INTEGER,              ///< Format as an integer
                   FORMAT_UNSIGNEDINTEGER,      ///< Format as an unsigned integer
                   FORMAT_TIME,                 ///< Format as a time - value must be in seconds
                   FORMAT_LOCAL_ENUMERATE,      ///< Format as a selection from the local enumerations set by setLocalEnumeration()
                   FORMAT_STRING,               ///< Format as a string
                   // Specials for specific PVA varient types
                   FORMAT_NT_TABLE,             ///< Format as a NTTable
                   FORMAT_NT_IMAGE,             ///< Format as a NTNDArray
                   FORMAT_OPAQUE                ///< Format as opaque, i.e. unknown/unhandled type.
               };

    /// \enum notations
    /// Notations when formatting a floating point number
    enum notations { NOTATION_FIXED      = QTextStream::FixedNotation,        ///< Standard floating point 123456.789
                     NOTATION_SCIENTIFIC = QTextStream::ScientificNotation,   ///< Scientific representation 1.23456789e6
                     NOTATION_AUTOMATIC  = QTextStream::SmartNotation         ///< Automatic choice of standard or scientific notation
                };    // WARNING keep these enumerations the same as QTextStream

    /// \num separators
    /// Defines the digit 'thousands' separator to be used.
    enum separators { SEPARATOR_NONE = 0,        ///< Use no separator,  e.g. 123456.123456789
                      SEPARATOR_COMMA,           ///< Use ',' as separator, e.g. 123,456.123,456,789
                      SEPARATOR_UNDERSCORE,      ///< Use '_' as separator, e.g. 123_456.123_456_789
                      SEPARATOR_SPACE            ///< Use ' ' as separator, e.g. 123 456.123 456 789
                    };

    /// \enum arrayActions
    /// What action to take when formatting array data
    enum arrayActions { APPEND, ///< Interpret each element in the array as an unsigned integer and append string representations of each element from the array with a space in between each.
                        ASCII,  ///< Interpret each element from the array as a character in a string. Translate all non printing characters to '?' except for trailing zeros (ignore them)
                        INDEX   ///< Interpret the element selected by setArrayIndex() as an unsigned integer
                    };

    // Construction
    explicit QEStringFormatting();
    ~QEStringFormatting();

    //===============================================
    // Main functions of this class:
    //   - Format a string based on a value
    //   - Translate a string and generate a value
    //===============================================
    QString formatString( const QVariant& value, int arrayIndex ) const;

    QVariant formatValue( const QString& text, bool& ok ) const;
    QVariant formatValue( const QVector<QString>& text, bool& ok ) const;


    // Functions to set up formatting information from the database
    void setDbEgu( QString egu );      // Units to be added (or removed) from the formatted string if 'addUnits' flag is set
    void setDbEnumerations( QStringList enumerations );
    void setDbPrecision( unsigned int dbPrecisionIn );

    // Functions to configure the formatting
    void setPrecision( int precision );
    void setUseDbPrecision( bool useDbPrecision );
    void setLeadingZero( bool leadingZero );
    void setTrailingZeros( bool trailingZeros );
    void setForceSign( bool forceSign );
    void setFormat( formats format );
    void setSeparator( const separators separator );
    void setRadix( const int radix );
    void setNotation( notations notation );
    void setArrayAction( arrayActions arrayActionIn );
    void setAddUnits( bool addUnits );
    void setLocalEnumeration( QString/*localEnumerationList*/ localEnumerationIn );

    // Functions to read the formatting configuration
    int          getPrecision() const;
    bool         getUseDbPrecision() const;
    bool         getLeadingZero() const;
    bool         getTrailingZeros() const;
    bool         getForceSign() const;
    formats      getFormat() const;
    separators   getSeparator () const;
    unsigned int getRadix() const;
    notations    getNotation() const;
    arrayActions getArrayAction() const;
    bool         getAddUnits() const;
    QString      getLocalEnumeration() const;
    QELocalEnumeration getLocalEnumerationObject() const;

private:
    // isNumeric set true iff value is numeric data.
    //
    QString formatElementString( const QVariant& value, bool& isNumeric ) const;

    // Type specific conversion functions
    // TODO: These SHOULD be rebadged formatToFloatingString etc.
    //
    void formatFromFloating( const QVariant& value ) const;
    void formatFromInteger( const QVariant& value ) const;
    void formatFromUnsignedInteger( const QVariant& value ) const;
    void formatFromTime( const QVariant& value ) const;
    void formatFromEnumeration( const QVariant& value ) const;
    void formatFromString( const QVariant &value ) const;

    // Utility functions
    void determineDbFormat( const QVariant &value ) const;
    void applyForceSign () const;
    QString insertSeparators( const QString& image ) const;
    QString realImage (const double item,
                       const notations notation,
                       const bool forceSign,
                       const int zeros,
                       const int prec) const;

    // Error reporting
    void formatFailure( QString message ) const;

    // Formatted output string
    mutable QTextStream stream;
    mutable QString outStr;
    mutable formats dbFormat;        // Format determined from read value (Floating, integer, etc).
    mutable bool dbFormatArray;      // True if read value is an array

    // Database information
    QString dbEgu;
    QStringList dbEnumerations;
    unsigned int dbPrecision;

    // Formatting configuration
    bool useDbPrecision;             // Use the number of decimal places displayed as defined in the database.
    bool leadingZero;                // Add a leading zero when required.
    bool trailingZeros;              // Add trailing zeros when required (up to the precision).
    bool forceSign;                  // Add "+" for numeric values >= 0
    formats format;                  // Presentation required (Floating, integer, etc).
    notations notation;              // Required notation for floating point formats
    separators separator;            // Thousands separator (applies to numeric values only)
    bool addUnits;                   // Flag use engineering units from database
    int precision;                   // Floating point precision. Used if 'useDbPrecision' is false.
    QELocalEnumeration localEnumerations;  // Local enumeration values.
    arrayActions arrayAction;       // Action to take when processing array or waveform data
};

#endif // QE_STRING_FORMATTING_H
