//  ***********************************************************************************************
//
//  Steca: stress and texture calculator
//
//! @file      core/loaders/load_tiff.cpp
//! @brief     Implements function loadTiff
//!
//! @homepage  https://github.com/scgmlz/Steca
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2016-2018
//! @authors   Scientific Computing Group at MLZ (see CITATION, MAINTAINER)
//
//  ***********************************************************************************************

#include "QCR/base/string_ops.h"
#include "core/base/exception.h"
#include "core/raw/rawfile.h"
#include <QDataStream>
#include <QDir>

namespace {

//! Reads one TIFF file.
static void loadTiff(
    Rawfile* file, const QString& filePath, deg phi, double monitor, double expTime)
{
    Metadata md;
    md.set("phi", phi);
    md.set("mon", monitor);
    md.set("t", expTime);

    // see http://www.fileformat.info/format/tiff/egff.htm

    QFile f{filePath};
    if (!(f.open(QFile::ReadOnly)))
        THROW("Cannot open file");

    QDataStream is(&f);
    is.setFloatingPointPrecision(QDataStream::SinglePrecision);

    auto check = [&is]() {
                     if (!(QDataStream::Ok == is.status()))
                         THROW("Could not read data from file"); };

    // magic
    qint16 magic;
    is >> magic;

    if (0x4949 == magic) // II - intel
        is.setByteOrder(QDataStream::LittleEndian);
    else if (0x4d4d == magic) // MM - motorola
        is.setByteOrder(QDataStream::BigEndian);
    else
        THROW("Bad magic bytes - not a TIFF file?");

    qint16 version;
    is >> version;
    if (42 != version)
        THROW("Bad version code");

    qint32 imageWidth = 0, imageHeight = 0, bitsPerSample = 0, sampleFormat = 0,
            rowsPerStrip = 0xffffffff, stripOffsets = 0, stripByteCounts = 0;

    qint16 tagId, dataType;
    qint32 dataCount, dataOffset;

    auto seek = [&f](qint64 offset) { if (!(f.seek(offset))) THROW("Bad offset"); };

    auto asUint = [&]() -> int {
        if (dataCount!=1)
            THROW("Bad data count");
        switch (dataType) {
        case 1: // byte
            return dataOffset & 0x000000ff; // some tif files did have trash there
        case 3: // short
            return dataOffset & 0x0000ffff;
        case 4: return dataOffset;
        }
        THROW("Invalid entry - not a simple number");
    };

    auto asStr = [&]()->QString {
        if (dataType!=2)
            THROW("Invalid entry - not a string");
        auto lastPos = f.pos();

        seek(dataOffset);
        QByteArray data = f.readLine(dataCount);
        seek(lastPos);

        return QString(data);
    };

    qint32 dirOffset;
    is >> dirOffset;
    check();
    seek(dirOffset);

    qint16 numDirEntries;
    is >> numDirEntries;

    for (int i=0; i<numDirEntries; ++i) {
        is >> tagId >> dataType >> dataCount >> dataOffset;
        check();

        switch (tagId) {
        // numbers
        case 256: imageWidth = asUint(); break;
        case 257: imageHeight = asUint(); break;
        case 258: bitsPerSample = asUint(); break;
        case 259: // Compression
            if (asUint()!=1)
                THROW("Unsupported flag value (compression=on)");
            break;
        case 273: stripOffsets = asUint(); break;
        case 277: // SamplesPerPixel
            if (asUint()!=1)
                THROW("Unsupported flag value (samplePerPixel!=1");
        case 278: rowsPerStrip = asUint(); break;
        case 279: stripByteCounts = asUint(); break;
        case 284: // PlanarConfiguration
            if (asUint()!=1)
                THROW("Unsupported flag value (planar=off)");
        case 339:
            sampleFormat = asUint(); // 1 unsigned, 2 signed, 3 IEEE
            break;

        // text
        case 269: // DocumentName
            md.set("comment", asStr());
            break;
        case 306: // DateTime
            md.set("date", asStr());
            break;
            //    default:
            //      TR("* NEW TAG *" << tagId << dataType << dataCount << dataOffset)
            //      break;
        }
    }

    if (imageWidth<=0)
        THROW("cannot read TIFF: unexpected imageWidth");
    if (imageHeight<=0)
        THROW("cannot read TIFF: unexpected imageHeight");
    if (stripOffsets<=0)
        THROW("cannot read TIFF: unexpected stripOffsets");
    if (stripByteCounts<=0)
        THROW("cannot read TIFF: unexpected stripByteCounts");
    if (imageHeight < rowsPerStrip)
        THROW("cannot read TIFF: imageHeight >= rowsPerStrip");

    if (sampleFormat<1 || sampleFormat>3)
        THROW("cannot read TIFF: unexpected sampleFormat");
    if (bitsPerSample!=32)
        THROW("cannot read TIFF: bitsPerSample!=32");


    size2d size(imageWidth, imageHeight);

    int count = imageWidth * imageHeight;
    std::vector<float> intens(count);

    if (!((bitsPerSample / 8) * count == stripByteCounts))
        THROW("cannot read TIFF: unexpected bitsPerSample");

    seek(stripOffsets);

    for (int i=0; i<intens.size(); ++i)
        switch (sampleFormat) {
        case 1: {
            qint32 sample;
            is >> sample;
            intens[i] = sample;
            break;
        }
        case 2: {
            qint32 sample;
            is >> sample;
            intens[i] = sample;
            break;
        }
        case 3: {
            float sample;
            is >> sample;
            intens[i] = sample;
            break;
        }
        }

    check();

    file->addDataset(std::move(md), size, std::move(intens));
}

} // namespace


namespace load {

//! Reads a .dat file, and returns contents as a Rawfile.

//! Called from function load_low_level(..) in file loaders.cpp.
//!
//! The .dat file is a digest that contains a list of TIFF files plus a few parameters.
//!
//! It has the following structure:
//!
//! ; comments
//!
//! ; file name, phi, monitor, Exposuretime  [the last two parameters are optional]
//!
//! Aus-Weimin-00001.tif -90
//! Aus-Weimin-00002.tif -85
//! Aus-Weimin-00003.tif -80
//! Aus-Weimin-00004.tif -75
//! Aus-Weimin-00005.tif -70
//! Aus-Weimin-00006.tif -65
//! Aus-Weimin-00007.tif -60
//! Aus-Weimin-00008.tif -55
//! Aus-Weimin-00009.tif -50

Rawfile loadTiffDat(const QString& filePath) {
    Rawfile ret(filePath);

    if (filePath=="")
        qFatal("BUG: call of loadTiffDat with empty argument");

    QFile f{filePath};
    if (!(f.open(QFile::ReadOnly)))
        THROW("Cannot open file "+filePath);

    QDir dir = QFileInfo(filePath).dir();

    QByteArray line;
    int iline = 0;
    while (!(line = f.readLine()).isEmpty()) {
        ++iline;
        QString s = line;

        // cut off comment
        int commentPos = s.indexOf(';');
        if (commentPos >= 0)
            s = s.left(commentPos);

        // split to parts
        if ((s = s.simplified()).isEmpty())
            continue;

        const QStringList lst = s.split(' ');
        const int cnt = lst.count();
        if (cnt<2 || cnt>4)
            THROW("File "+filePath+": bad metadata format");

        // file, phi, monitor, expTime
        bool ok;
        QString tiffFileName = lst.at(0);
        deg phi = lst.at(1).toDouble(&ok);
        if (!(ok))
            THROW("File "+filePath+": bad phi value");

        double monitor = 0;
        if (cnt > 2) {
            monitor = lst.at(2).toDouble(&ok);
            if (!(ok))
                THROW("File "+filePath+": bad monitor value");
        }

        double expTime = 0;
        if (cnt > 3) {
            expTime = lst.at(3).toDouble(&ok);
            if (!(ok))
                THROW("File "+filePath+": bad expTime value");
        }

        try {
            // load one dataseq
            loadTiff(&ret, dir.filePath(tiffFileName), phi, monitor, expTime);
        } catch (const Exception& ex) {
            THROW("File "+filePath+": cannot load image number "+qcr::str::to_s(iline)
                  +" ("+tiffFileName + "): " + ex.msg());
        }
    }

    return ret;
}

} // namespace load
