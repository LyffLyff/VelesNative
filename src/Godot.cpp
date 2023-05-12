#include <Godot.hpp>
#include <StreamPeerBuffer.hpp>
#include <Reference.hpp>
#include <taglib.h>
#include <taglib/fileref.h>
#include <id3v2tag.h>
#include <id3v2header.h>
#include <id3v2frame.h>
#include <mpegfile.h>
#include <attachedpictureframe.h>
#include <audioproperties.h>
#include <mp4file.h>
#include <mp4tag.h>
#include <mp4coverart.h>
#include <vorbisproperties.h>
#include <wavproperties.h>
#include <mp4properties.h>
#include <speexproperties.h>
#include <opusproperties.h>
#include <apeproperties.h>
#include <flacproperties.h>
#include <audioproperties.h>
#include <flacfile.h>
#include <flacpicture.h>
#include <mp4properties.h>
#include <vorbisfile.h>
#include <synchronizedlyricsframe.h>
#include <unsynchronizedlyricsframe.h>
#include <textidentificationframe.h>
#include <popularimeterframe.h>
#include <tpropertymap.h>
#include <fstream>

// Global Variables
std::map<std::string, std::string> field_identifiers = {
    {"COMPOSER", "TCOM"},
    {"ALBUM", "TALB"},
    {"TITLE", "TIT2"},
    {"ARTIST", "TPE1"},
    {"ALBUMARTIST", "TPE2"},
    {"CONDUCTOR", "TPE3"},
    {"TRACKNUMBER", "TRCK"},
    {"DISCNUMBER", "TPOS"},
    {"TOTALDISCS", "TPOS"},
    {"TOTALTRACKS", "TRCK"},
    {"TITLESORTORDER", "TSOT"},
    {"ALBUMSORTORDER", "TSOA"},
    {"ORIGALBUM", "TOAL"},
    {"SUBTITLE", "TIT3"},
    {"SETSUBTITLE", "TSST"},
    {"PERFORMERSORTORDER", "TSOP"},
    {"WWWARTIST", "WOAR"},
    {"ORIGARTIST", "TOPE"},
    {"BAND", "TPE2"},
    {"LYRICIST", "TEXT"},
    {"ORIGLYRICIST", "TOLY"},
    {"MIXARTIST", "TPE4"},
    {"PUBLISHER", "TPUB"},
    {"WWWPUBLISHER", "WPUB"},
    {"ENCODEDBY", "TENC"},
    {"INVOLVEDPEOPLE2", "TIPL"},
    {"MUSICIANCREDITLIST", "TMCL"},
    {"GENRE", "TCON"},
    {"MOOD", "TMOO"},
    {"MEDIATYPE", "TMED"},
    {"WWWAUDIOSOURCE", "WOAS"},
    {"LANGUAGE", "TLAN"},
    {"CONTENTGROUP", "TIT1"},
    {"RECORDINGTIME", "TDRC"},
    {"RELEASETIME", "TDRL"},
    {"ENCODINGTIME", "TDEN"},
    {"TAGGINGTIME", "TDTG"},
    {"ORIGRELEASETIME", "TDOR"},
    {"COMMENT", "COMM"},
    {"POPULARIMETER", "POPM"},
    {"PLAYCOUNTER", "PCNT"},
    {"ENCODERSETTINGS", "TSSE"},
    {"SONGLEN", "TLEN"},
    {"FILETYPE", "TFLT"},
    {"SEEKFRAME", "SEEK"},
    {"MPEGLOOKUP", "MLLT"},
    {"AUDIOSEEKPOINT", "ASPI"},
    {"PLAYLISTDELAY", "TDLY"},
    {"EVENTTIMING", "ETCO"},
    {"SYNCEDTEMPO", "SYTC"},
    {"POSITIONSYNC", "POSS"},
    {"BUFFERSIZE", "RBUF"},
    {"BPM", "TBPM"},
    {"INITIALKEY", "TKEY"},
    {"VOLUMEADJ2", "RVA2"},
    {"REVERB", "RVRB"},
    {"EQUALIZATION2", "EQU2"},
    {"ISRC", "TSRC"},
    {"CDID", "MCDI"},
    {"UNIQUEFILEID", "UFID"},
    {"WWWCOMMERCIALINFO", "WCOM"},
    {"WWWPAYMENT", "WPAY"},
    {"FILEOWNER", "TOWN"},
    {"COMMERCIAL", "COMR"},
    {"COPYRIGHT", "TCOP"},
    {"PRODUCEDNOTICE", "TPRO"},
    {"TERMSOFUSE", "USER"},
    {"WWWCOPYRIGHT", "WCOP"},
    {"OWNERSHIP", "OWNE"},
    {"AUDIOCRYPTO", "AENC"},
    {"CRYPTOREG", "ENCR"},
    {"GROUPINGREG", "GRID"},
    {"SIGNATURE", "SIGN"},
    {"UNSYNCEDLYRICS", "USLT"},
    {"SYNCEDLYRICS", "SYLT"},
    {"PICTURE", "APIC"},
    {"GENERALOBJECT", "GEOB"},
    {"PRIVATE", "PRIV"},
    {"USERTEXT", "TXXX"},
    {"LINKEDINFO", "LINK"},
    {"ORIGFILENAME", "TOFN"},
    {"NETRADIOSTATION", "TRSN"},
    {"NETRADIOOWNER", "TRSO"},
    {"WWWRADIOPAGE", "WORS"},
    {"WWWUSER", "WXXX"},
    {"WWWAUDIOFILE", "WOAF"}
};


enum {
    PNG,
    JPG,
    JFIF,
    WEBP,
    BMP,
    GIF,
};


// Prototypes


godot::PoolByteArray ByteVector2PoolByte(TagLib::ByteVector&& data) {
    godot::PoolByteArray converted_buffer = godot::PoolByteArray();
    const  char* original_buffer = data.data();
    converted_buffer.resize(data.size());
    memcpy((uint8_t*)converted_buffer.write().ptr(), original_buffer, data.size());
    return converted_buffer;
}


unsigned int identify_image_from_header(TagLib::ByteVector image_header) {
    std::map < const char*, int> file_signatures{
       {"89504e470d0a1a0a", PNG},           // PNG
       {"ffd8ff", JPG},                     // JPEG
       {"ffd8ffe000104a4649460001", JFIF},  // JFIF / JPEG
       {"57454250", WEBP},                  // WEBP
       {"52494646", WEBP},                  // RIFF / WEBP
       {"424d", BMP},                       // BMP
       {"474946383761", GIF},               // GIF (87a)
       {"474946383961", GIF}                // GIF (89a)
    };

    godot::PoolByteArray x = ByteVector2PoolByte(image_header.data());
    godot::String hex_header = godot::String::hex_encode_buffer(x.read().ptr(), x.size());

    for (auto it = file_signatures.begin(); it != file_signatures.end(); ++it) {
        // it->first = key
        if (hex_header.begins_with_char_array(it->first)) {
            // returns file extension if corresponding signature was found
            return file_signatures[it->first];
        }
    }

    return PNG;
}


godot::String get_extension_from_header(TagLib::ByteVector image_header) {
    std::map < const char*, godot::String> file_signatures{
        {"89504e470d0a1a0a", ".png"},           // PNG
        {"ffd8ff", ".jpg"},                     // JPEG
        {"ffd8ffe000104a4649460001", ".jpg"},   // JFIF / JPEG
        {"57454250", ".webp"},                  // WEBP
        {"52494646", ".webp"},                  // RIFF / WEBP
        {"424d", ".bmp"},                       // BMP
        {"474946383761", ".gif"},               // GIF (87a)
        {"474946383961", ".gif"}                // GIF (89a)
    };

    godot::PoolByteArray x = ByteVector2PoolByte(image_header.data());
    godot::String hex_header = godot::String::hex_encode_buffer(x.read().ptr(), x.size());
    for (auto it = file_signatures.begin(); it != file_signatures.end(); ++it) {
        // it->first = key
        if (hex_header.begins_with_char_array(it->first)) {
            // returns file extension if corresponding signature was found
            return file_signatures[it->first];
        }
    }

    return ".jpg";
}


godot::String get_mime_type(godot::PoolByteArray image_header) {
    std::map < const char*, godot::String> file_signatures{
       {"89504e470d0a1a0a", "image/png"},           // PNG
       {"ffd8ff", "image/jpeg"},                    // JPEG
       {"ffd8ffe000104a4649460001", "image/jpeg"},  // JFIF / JPEG
       {"57454250", "image/webp"},                  // WEBP
       {"52494646", "image/webp"},                  // RIFF / WEBP
       {"424d", "image/bmp"},                       // BMP
       {"474946383761", "image/gif"},               // GIF (87a)
       {"474946383961", "image/gif"}                // GIF (89a)
    };

    godot::String hex_header = godot::String::hex_encode_buffer(image_header.read().ptr(), image_header.size());
    for (auto it = file_signatures.begin(); it != file_signatures.end(); ++it) {
        // it->first = key
        if (hex_header.begins_with_char_array(it->first)) {
            // returns file extension if corresponding signature was found
            return file_signatures[it->first];
        }
    }

    return "type unknown";
}


TagLib::FileName gd_string_to_filename(godot::String filepath) {
#ifdef _WIN32
    return TagLib::FileName(filepath.unicode_str());
#endif //
    return TagLib::FileName(filepath.alloc_c_string());
}


static TagLib::String get_extension(TagLib::String file_path) {
    TagLib::String x = file_path.substr(file_path.size() - 3);
    x = x.upper();
    return  x;
}


godot::String add_number_to_string(godot::String filepath, unsigned short number) {
    if (number == 0) { return filepath; }
    return filepath + "[" + godot::String::num_int64(number) + "]";
}


bool save_buffer_to_file(godot::String dst_path, TagLib::ByteVector data) {
    std::ofstream out(dst_path.alloc_c_string(), std::ios::binary);
    out.write(data.data(), data.size());
    out.close();
    return true;
}


TagLib::FLAC::Picture* create_flac_picture(TagLib::ByteVector&& image_data, TagLib::String mime_type) {
    TagLib::FLAC::Picture* new_picture = new TagLib::FLAC::Picture;
    new_picture->setData(image_data);
    new_picture->setMimeType(mime_type);
    return new_picture;
}


TagLib::ID3v2::AttachedPictureFrame* create_apic_frame(TagLib::ByteVector&& data, TagLib::String mime_type) {
    TagLib::ID3v2::AttachedPictureFrame* new_frame = new TagLib::ID3v2::AttachedPictureFrame;
    new_frame->setMimeType(mime_type);
    new_frame->setPicture(data);
    return new_frame;
}


class ImageFile : public TagLib::File
{
public:

    ImageFile(TagLib::FileName file) : TagLib::File(file) {

    }

    TagLib::ByteVector data() {
        return readBlock(length());
    }


private:
    virtual TagLib::Tag* tag() const { return 0; }
    virtual TagLib::AudioProperties* audioProperties() const { return 0; }
    virtual bool save() { return false; }
};


class OGG_FLAC : public godot::Reference {
    GODOT_CLASS(OGG_FLAC, godot::Reference)
        std::vector<std::string> property_ids = {
            "COMPOSER",
            "ALBUM",
            "TITLE",
            "ARTIST",
            "ALBUMARTIST",
            "CONDUCTOR",
            "TRACKNUMBER",
            "DISCNUMBER",
            "TOTALDISCS",
            "TOTALTRACKS",
            "TITLESORTORDER",
            "ALBUMSORTORDER",
            "ORIGALBUM",
            "SUBTITLE",
            "SETSUBTITLE",
            "PERFORMERSORTORDER",
            "WWWARTIST",
            "ORIGARTIST",
            "BAND",
            "LYRICIST",
            "ORIGLYRICIST",
            "MIXARTIST",
            "PUBLISHER",
            "WWWPUBLISHER",
            "ENCODEDBY",
            "INVOLVEDPEOPLE2",
            "MUSICIANCREDITLIST",
            "GENRE",
            "MOOD",
            "MEDIATYPE",
            "WWWAUDIOSOURCE",
            "LANGUAGE",
            "CONTENTGROUP",
            "RECORDINGTIME",
            "RELEASETIME",
            "ENCODINGTIME",
            "TAGGINGTIME",
            "ORIGRELEASETIME",
            "COMMENT",
            "POPULARIMETER",
            "PLAYCOUNTER",
            "ENCODERSETTINGS",
            "SONGLEN",
            "FILETYPE",
            "SEEKFRAME",
            "MPEGLOOKUP",
            "AUDIOSEEKPOINT",
            "PLAYLISTDELAY",
            "EVENTTIMING",
            "SYNCEDTEMPO",
            "POSITIONSYNC",
            "BUFFERSIZE",
            "BPM",
            "INITIALKEY",
            "VOLUMEADJ2",
            "REVERB",
            "EQUALIZATION2",
            "ISRC",
            "CDID",
            "UNIQUEFILEID",
            "WWWCOMMERCIALINFM",
            "WWWPAYMENT",
            "FILEOWNER",
            "COMMERCIAL",
            "COPYRIGHT",
            "PRODUCEDNOTICE",
            "TERMSOFUSE",
            "WWWCOPYRIGHT",
            "OWNERSHIP",
            "AUDIOCRYPTO",
            "CRYPTOREG",
            "GROUPINGREG",
            "SIGNATURE",
            "UNSYNCEDLYRICS",
            "SYNCEDLYRICS"
            "PICTURE",
            "GENERALOBJECT",
            "PRIVATE",
            "USERTEXT",
            "LINKEDINFO",
            "ORIGFILENAME",
            "NETRADIOSTATION",
            "NETRADIOOWNER",
            "WWWRADIOPAGE",
            "WWWUSER",
            "WWWAUDIOFILE"
    };
public:
    static void _register_methods() {}
    void _init() {}
};


class AudioProperties : public godot::Reference {
    GODOT_CLASS(AudioProperties, godot::Reference)

public:
    static void _register_methods() {
        register_method("get_duration_seconds", &AudioProperties::get_duration_seconds);
        register_method("get_channels", &AudioProperties::get_channels);
        register_method("get_bitrate", &AudioProperties::get_bitrate);
        register_method("get_sample_rate", &AudioProperties::get_sample_rate);
    }

    void _init() {
    }

    int get_duration_seconds(godot::String path) {
        TagLib::FileRef audioFile(gd_string_to_filename(path));
        if (audioFile.isNull()) {
            return 0;
        }
        //Returns length of the given Audio File in Seconds
        return audioFile.audioProperties()->lengthInSeconds();
    }

    int get_channels(godot::String path) {
        TagLib::FileRef audioFile(gd_string_to_filename(path));
        if (audioFile.isNull()) {
            return 0;
        }
        //Returns length of the given Audio File in Seconds
        return audioFile.audioProperties()->channels();
    }

    int get_bitrate(godot::String path) {
        TagLib::FileRef audioFile(gd_string_to_filename(path));
        if (audioFile.isNull()) {
            return 0;
        }
        //Returns length of the given Audio File in Seconds
        return audioFile.audioProperties()->bitrate();
    }

    int get_sample_rate(godot::String path) {
        TagLib::FileRef audioFile(gd_string_to_filename(path));
        if (audioFile.isNull()) {
            return 0;
        }
        //Returns length of the given Audio File in Seconds
        return audioFile.audioProperties()->sampleRate();
    }
};


class MPEG : public godot::Reference {
    GODOT_CLASS(MPEG, godot::Reference)

public:
    static void _register_methods() {
        register_method("get_cover", &MPEG::get_cover);
        register_method("get_covers", &MPEG::get_covers);
        register_method("get_text_frame_ids", &MPEG::get_text_frame_ids);
        register_method("add_cover", &MPEG::add_cover);
        register_method("remove_cover", &MPEG::remove_cover);
        register_method("remove_all_covers", &MPEG::remove_all_covers);
        register_method("copy_cover", &MPEG::copy_cover);
        register_method("get_embedded_cover_count", &MPEG::get_embedded_cover_count);
        register_method("remove_tag", &MPEG::remove_tag);
        register_method("export_embedded_covers", &MPEG::export_embedded_covers);
        register_method("remove_text_frame", &MPEG::remove_text_frame);
        register_method("set_cover_type", &MPEG::set_cover_type);
        register_method("get_cover_type", &MPEG::get_cover_type);
        register_method("set_text_frames", &MPEG::set_text_frames);
        register_method("get_single_text_frame", &MPEG::get_single_text_frame);
    }

    void _init() {
    }

    godot::PoolByteArray get_cover(godot::String audio_filepath, unsigned int cover_idx) {
        // returns a specific cover, decided by a given index

        TagLib::MPEG::File mpeg_file(gd_string_to_filename(audio_filepath));
        if (!mpeg_file.isValid()) { return godot::PoolByteArray(); }

        TagLib::ID3v2::Tag* mpeg_tag = mpeg_file.ID3v2Tag(true);
        if (mpeg_tag == NULL) { return godot::PoolByteArray(); }

        unsigned int counter = 0;
        TagLib::ID3v2::FrameList embedded_covers = mpeg_tag->frameListMap()["APIC"];
        for (TagLib::ID3v2::FrameList::ConstIterator it = embedded_covers.begin(); it != embedded_covers.end(); it++) {
            if (counter == cover_idx) {
                return ByteVector2PoolByte(static_cast<TagLib::ID3v2::AttachedPictureFrame*> (*it)->picture());
            }
            counter += 1;
        }

        return godot::PoolByteArray();
    }

    godot::Array get_covers(godot::String audio_filepath) {
        // returns the data of all attached covers as an Array of ByteArrays
        TagLib::MPEG::File mpeg_file(gd_string_to_filename(audio_filepath));
        if (!mpeg_file.isValid()) { return godot::Array(); }

        TagLib::ID3v2::Tag* mpeg_tag = mpeg_file.ID3v2Tag();
        if (mpeg_tag->isEmpty()) { return godot::Array(); }

        godot::Array cover_data = godot::Array();
        TagLib::ID3v2::FrameList embedded_covers = mpeg_tag->frameListMap()["APIC"];
        for (TagLib::ID3v2::FrameList::ConstIterator it = embedded_covers.begin(); it != embedded_covers.end(); it++) {
            // converts the standard frame to an APIC-Frame and adds the pure image data as PoolByteArray
            // rendering standard frame returns data WITH the specifics frame header, thats why the conversion
            cover_data.push_back(ByteVector2PoolByte((static_cast<TagLib::ID3v2::AttachedPictureFrame*> (*it)->picture())));
        }

        return cover_data;
    }


    bool add_cover(godot::String audio_filepath, godot::String image_filepath, godot::String mime_type) {
        // appends a cover to the file

        TagLib::MPEG::File mpeg_file(gd_string_to_filename(audio_filepath));
        if (!mpeg_file.isValid()) { return false; }

        TagLib::ID3v2::Tag* mpeg_tag = mpeg_file.ID3v2Tag(true);

        ImageFile image = ImageFile(gd_string_to_filename(image_filepath));
        if (!image.isOpen()) { return false; }

        mpeg_tag->addFrame(create_apic_frame(image.data(), mime_type.alloc_c_string()));

        return mpeg_file.save();
    }


    bool remove_all_covers(godot::String audio_filepath) {
        TagLib::MPEG::File mpeg_file(gd_string_to_filename(audio_filepath));
        if (!mpeg_file.isOpen()) { return false; }

        TagLib::ID3v2::Tag* mpeg_tag = mpeg_file.ID3v2Tag();
        if (mpeg_tag->isEmpty()) { return false; }

        mpeg_tag->removeFrames("APIC");
        return mpeg_file.save();;
    }


    bool remove_cover(godot::String audio_filepath, unsigned int cover_idx) {
        TagLib::MPEG::File mpeg_file(gd_string_to_filename(audio_filepath));
        if (!mpeg_file.isValid() || !mpeg_file.hasID3v2Tag()) { return false; }
        TagLib::ID3v2::Tag* mpeg_tag = mpeg_file.ID3v2Tag();
        unsigned int counter = 0;
        TagLib::ID3v2::FrameList embedded_covers = mpeg_tag->frameListMap()["APIC"];
        for (TagLib::ID3v2::FrameList::ConstIterator it = embedded_covers.begin(); it != embedded_covers.end(); it++) {
            if (counter == cover_idx) {
                mpeg_tag->removeFrame(*it);
                break;
            }
            counter += 1;
        }
        return mpeg_file.save();
    }


    bool export_embedded_covers(godot::String audio_filepath, godot::String dst_filepath) {
        godot::Godot::print(audio_filepath);
        godot::Godot::print(dst_filepath);
        TagLib::MPEG::File mpeg_file(gd_string_to_filename(audio_filepath));
        if (!mpeg_file.isValid()) { return false; }

        if (!mpeg_file.hasID3v2Tag()) { return false; }
        TagLib::ID3v2::Tag* mpeg_tag = mpeg_file.ID3v2Tag();

        TagLib::ID3v2::FrameList embedded_covers = mpeg_tag->frameListMap()["APIC"];//look for picture frames only
        unsigned int counter = 0;
        const int BUFFER_SIZE = 256;
        for (TagLib::ID3v2::FrameList::ConstIterator it = embedded_covers.begin(); it != embedded_covers.end(); it++) {
            TagLib::ByteVector image_data = static_cast<TagLib::ID3v2::AttachedPictureFrame*> (*it)->picture();
            TagLib::ByteVector temp_header;
            temp_header.resize(BUFFER_SIZE);
            memcpy(temp_header.data(), image_data.data(), BUFFER_SIZE);
            // with added number
            if (!save_buffer_to_file(add_number_to_string(dst_filepath, counter) + get_extension_from_header(temp_header), image_data)) {
                return false;
            }
            counter++;
        }
        return true;
    }


    bool copy_cover(godot::String audio_filepath, godot::String dst_filepath, unsigned int cover_idx) {
        TagLib::MPEG::File mpeg_file(gd_string_to_filename(audio_filepath));

        if (!mpeg_file.isValid()) { return false; }
        TagLib::ID3v2::Tag* mpeg_tag = mpeg_file.ID3v2Tag();

        if (mpeg_tag == NULL) { return false; }
        TagLib::ID3v2::FrameList listOfMp3Frames = mpeg_tag->frameListMap()["APIC"];//look for picture frames only

        if (listOfMp3Frames.isEmpty()) { return false; }

        TagLib::ByteVector image_data = static_cast<TagLib::ID3v2::AttachedPictureFrame*> (listOfMp3Frames[cover_idx])->picture();

        return save_buffer_to_file(dst_filepath, image_data);
    }


    unsigned int get_embedded_cover_count(godot::String audio_filepath) {
        // returns the amount of embedded covers within file
        TagLib::MPEG::File mpeg_file(gd_string_to_filename(audio_filepath));
        if (!mpeg_file.isValid() || !mpeg_file.hasID3v2Tag()) { return  0; }

        TagLib::ID3v2::Tag* mpeg_tag = mpeg_file.ID3v2Tag();
        return mpeg_tag->frameListMap()["APIC"].size();
    }


    bool set_text_frames(godot::String mp3_filepath, godot::PoolStringArray frame_ids, godot::PoolStringArray values) {
        TagLib::MPEG::File mp3_file(gd_string_to_filename(mp3_filepath));
        if (!mp3_file.isValid() || mp3_file.readOnly()) {
            godot::Godot::print_error("Error opening MP3 file", __FUNCTION__, __FILE__, __LINE__);
            return false;
        }

        TagLib::ID3v2::Tag* tag = mp3_file.ID3v2Tag(true);

        char* temp_id;
        char* temp_value;

        for (size_t i = 0; i < frame_ids.size(); i++) {
            temp_id = frame_ids[i].alloc_c_string();
            temp_value = values[i].alloc_c_string();
            TagLib::ID3v2::FrameList frames = tag->frameList(temp_id);
            if (frames.isEmpty()) {
                TagLib::ID3v2::Frame* new_frame = new TagLib::ID3v2::TextIdentificationFrame(temp_id, TagLib::String::UTF8);
                new_frame->setText(temp_value);
                tag->addFrame(new_frame);
                godot::Godot::print(temp_value);
                godot::Godot::print(temp_id);
            }
            else {
                TagLib::ID3v2::Frame* frame = frames.front();
                frame->setText(temp_value);
            }
        }

        return mp3_file.save();
    }


    void set_cover_type(godot::String audio_filepath, int cover_idx, unsigned int cover_type_idx) {
        TagLib::MPEG::File file(gd_string_to_filename(audio_filepath));
        if (!file.isValid() || !file.hasID3v2Tag()) {
            return;
        }
        TagLib::ID3v2::Tag* tag = file.ID3v2Tag();
        TagLib::ID3v2::FrameList frames = tag->frameList("APIC");
        int count = 0;
        for (TagLib::ID3v2::FrameList::ConstIterator it = frames.begin(); it != frames.end(); ++it) {
            TagLib::ID3v2::AttachedPictureFrame* coverFrame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(*it);
            if (coverFrame) {
                if (count == cover_idx) {
                    coverFrame->setType(TagLib::ID3v2::AttachedPictureFrame::Type(cover_type_idx));
                    file.save();
                    return;
                }
                count++;
            }
        }
    }


    int get_cover_type(godot::String audio_filepath, int cover_idx) {
        TagLib::MPEG::File file(gd_string_to_filename(audio_filepath));
        if (!file.isValid() || !file.hasID3v2Tag()) {
            return -1;
        }
        TagLib::ID3v2::Tag* tag = file.ID3v2Tag();
        TagLib::ID3v2::FrameList frames = tag->frameList("APIC");
        int count = 0;
        for (TagLib::ID3v2::FrameList::ConstIterator it = frames.begin(); it != frames.end(); ++it) {
            if (count == cover_idx) {
                return static_cast<int>(dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(*it)->type());
            }
            count++;
        }
        return -1;
    }


    godot::Dictionary get_text_properties(godot::String audio_filepath) {
        // Returns all SET text tags within the given files

        TagLib::MPEG::File mpeg_file(gd_string_to_filename(audio_filepath));
        if (!mpeg_file.isValid()) { return godot::Dictionary(); }

        TagLib::ID3v2::Tag* mpeg_tag = mpeg_file.ID3v2Tag(true);
        if (!mpeg_tag) { return godot::Dictionary(); }

        godot::Dictionary properties = {};

        TagLib::ID3v2::FrameListMap frame_map = mpeg_tag->frameListMap();
        TagLib::ID3v2::FrameList temp_frames;

        for (auto it = field_identifiers.begin(); it != field_identifiers.end(); it++) {
            // Get Frame Data
            temp_frames = frame_map[field_identifiers[it->first].c_str()];

            // Append String representation of frame if it exists
            if (temp_frames.size() > 0) {
                godot::Godot::print(it->first.c_str());
                properties[field_identifiers[it->first].c_str()] = temp_frames[0]->toString().toCString(true);
            }
        }

        return properties;
    }


    godot::String get_single_text_frame(godot::String audio_filepath, godot::String frame_id) {
        godot::String frame_value = "";

        TagLib::MPEG::File mpeg_file(gd_string_to_filename(audio_filepath));
        if (!mpeg_file.isValid()) { return frame_value; }

        TagLib::ID3v2::Tag* mpeg_tag = mpeg_file.ID3v2Tag(true);
        if (!mpeg_tag) { return frame_value; }

        TagLib::ID3v2::FrameList id_frames = mpeg_tag->frameListMap()[frame_id.alloc_c_string()];

        if (id_frames.size() > 0) {
            godot::Godot::print("FOUND SOMETHINGF");
            frame_value = id_frames[0]->toString().toCString(true);
        }
        else {
            godot::Godot::print("FOUND NOTHING");
        }


        return frame_value;
    }


    void remove_text_frame(godot::String audio_filepath, godot::String frame_id) {
        TagLib::MPEG::File mpeg_file(gd_string_to_filename(audio_filepath));
        if (!mpeg_file.isValid()) { return; }

        TagLib::ID3v2::Tag* mpeg_tag = mpeg_file.ID3v2Tag(true);
        if (!mpeg_tag) { return; }

        TagLib::ID3v2::FrameListMap frame_map = mpeg_tag->frameListMap();

        godot::Godot::print("REMOEOEOEMEOMEO");

        if (frame_map[frame_id.alloc_c_string()].size() > 0) {
            godot::Godot::print("REMOEOEOEMEOMEO");
            mpeg_tag->removeFrame(frame_map[frame_id.alloc_c_string()][0], true);
        }

        mpeg_file.save();
    }


    bool remove_tag(godot::String audio_filepath) {
        TagLib::MPEG::File mpeg_file(gd_string_to_filename(audio_filepath));
        if (!mpeg_file.isValid()) { return false; }
        mpeg_file.strip(TagLib::MPEG::File::AllTags, true);
        return mpeg_file.save();
    }


    godot::PoolStringArray get_text_frame_ids(godot::String audio_filepath) {
        godot::Dictionary used_properties = get_text_properties(audio_filepath);
        godot::PoolStringArray unused_properties = godot::PoolStringArray();
        for (auto it = field_identifiers.begin(); it != field_identifiers.end(); it++) {
            unused_properties.push_back(it->second.c_str());
        }
        return unused_properties;
    }
};


class OGG_VORBIS : public OGG_FLAC {
    GODOT_CLASS(OGG_VORBIS, OGG_FLAC)

public:
    static void _register_methods() {
        register_method("get_cover", &OGG_VORBIS::get_cover);
        register_method("get_covers", &OGG_VORBIS::get_covers);
        register_method("add_cover", &OGG_VORBIS::add_cover);
        register_method("copy_cover", &OGG_VORBIS::copy_cover);
        register_method("remove_cover", &OGG_VORBIS::remove_cover);
        register_method("remove_all_covers", &OGG_VORBIS::remove_all_covers);
        register_method("get_embedded_cover_count", &OGG_VORBIS::get_embedded_cover_count);
        register_method("remove_tag", &OGG_VORBIS::remove_tag);
        register_method("set_field", &OGG_VORBIS::set_field);
        register_method("get_all_fields", &OGG_VORBIS::get_all_fields);
        register_method("get_single_field_property", &OGG_VORBIS::get_single_field_property);
    }

    void _init() {
    }

    bool add_cover(godot::String audio_filepath, godot::String image_filepath, godot::String mime_type) {
        TagLib::Ogg::Vorbis::File vorbis_file(gd_string_to_filename(audio_filepath));

        if (!vorbis_file.isOpen()) { return false; }
        TagLib::Ogg::XiphComment* vorbis_tag = vorbis_file.tag();

        ImageFile image = ImageFile(gd_string_to_filename(image_filepath));
        if (!image.isOpen()) { return false; }

        vorbis_tag->addPicture(create_flac_picture(image.data(), mime_type.alloc_c_string()));
        return vorbis_file.save();
    }


    godot::PoolByteArray get_cover(godot::String audio_filepath, unsigned int cover_idx) {
        // returns the data of the cover at the given index
        TagLib::Ogg::Vorbis::File vorbis_file(gd_string_to_filename(audio_filepath));
        if (!vorbis_file.isOpen()) { return godot::PoolByteArray(); }

        TagLib::Ogg::XiphComment* vorbis_tag = vorbis_file.tag();
        if (!vorbis_tag) { return godot::PoolByteArray(); }

        unsigned int counter = 0;
        TagLib::List<TagLib::FLAC::Picture*> pictures = vorbis_tag->pictureList();
        for (TagLib::List<TagLib::FLAC::Picture*>::ConstIterator it = pictures.begin(); it != pictures.end(); it++) {
            if (counter == cover_idx) {
                return ByteVector2PoolByte((*it)->data());
            }
            counter += 1;
        }
        return godot::PoolByteArray();
    }


    godot::Array get_covers(godot::String audio_filepath) {
        TagLib::Ogg::Vorbis::File vorbis_file(gd_string_to_filename(audio_filepath));
        if (!vorbis_file.isValid()) { return godot::Array(); }

        TagLib::Ogg::XiphComment* vorbis_tag = vorbis_file.tag();
        if (vorbis_tag->isEmpty()) { return godot::Array(); }

        godot::Array cover_data = godot::Array();
        TagLib::List<TagLib::FLAC::Picture*> pictures = vorbis_tag->pictureList();
        for (TagLib::List<TagLib::FLAC::Picture*>::ConstIterator it = pictures.begin(); it != pictures.end(); it++) {
            cover_data.push_back(ByteVector2PoolByte((*it)->data()));
        }
        return cover_data;
    }


    bool remove_all_covers(godot::String audio_filepath) {

        TagLib::Ogg::Vorbis::File vorbis_file(gd_string_to_filename(audio_filepath));
        if (!vorbis_file.isOpen()) { return false; }

        TagLib::Ogg::XiphComment* vorbis_tag = vorbis_file.tag();
        if (vorbis_tag->isEmpty()) { return false; }

        vorbis_tag->removeAllPictures();
        return vorbis_file.save();
    }


    bool remove_cover(godot::String audio_filepath, unsigned int cover_idx) {

        TagLib::Ogg::Vorbis::File vorbis_file(gd_string_to_filename(audio_filepath));
        if (!vorbis_file.isOpen()) { return false; }

        TagLib::Ogg::XiphComment* vorbis_tag = vorbis_file.tag();
        if (vorbis_tag->isEmpty()) { return false; }

        // removing a specific index of a an embedded picture within the file
        TagLib::List<TagLib::FLAC::Picture*> pictures = vorbis_tag->pictureList();  // .begin() and .end() iterators have to be called by the same list
        unsigned int counter = 0;
        for (TagLib::List<TagLib::FLAC::Picture*>::ConstIterator it = pictures.begin(); it != pictures.end(); it++) {
            if (counter == cover_idx) {
                vorbis_tag->removePicture(*it, true);
                break;
            }
            counter += 1;
        }

        return vorbis_file.save();
    }


    bool copy_cover(godot::String audio_filepath, godot::String dst_filepath, unsigned int cover_idx) {
        TagLib::Ogg::Vorbis::File vorbis_file(gd_string_to_filename(audio_filepath));
        if (!vorbis_file.isOpen()) { return false; }

        TagLib::Ogg::XiphComment* vorbis_tag = vorbis_file.tag();
        if (vorbis_tag->isEmpty()) { return false; }

        auto embedded_covers = vorbis_tag->pictureList();
        auto cover = NULL;
        unsigned int counter = 0;
        for (TagLib::List<TagLib::FLAC::Picture*>::ConstIterator it = embedded_covers.begin(); it != embedded_covers.end(); it++) {
            if (counter == cover_idx) {
                return save_buffer_to_file(dst_filepath, (*it)->data());
            }
        }
        return false;
    }


    unsigned int get_embedded_cover_count(godot::String audio_filepath) {
        // returns the amount of embedded covers within file
        TagLib::Ogg::Vorbis::File vorbis_file(gd_string_to_filename(audio_filepath));
        if (!vorbis_file.isValid()) { return 0; };

        TagLib::Ogg::XiphComment* vorbis_tag = vorbis_file.tag();
        if (vorbis_tag == NULL) { return 0; };

        return vorbis_tag->pictureList().size();
    }

    bool set_field(godot::String audio_filepath, godot::String field_id, godot::String field_data) {
        TagLib::Ogg::Vorbis::File vorbis_file(gd_string_to_filename(audio_filepath));
        if (!vorbis_file.isValid()) { return false; };

        TagLib::Ogg::XiphComment* vorbis_tag = vorbis_file.tag();
        if (vorbis_tag == NULL) { return false; };

        vorbis_tag->addField(field_id.alloc_c_string(), field_data.alloc_c_string(), true);

        return vorbis_file.save();
    }


    godot::Dictionary get_all_fields(godot::String audio_filepath) {
        godot::Dictionary fields = {};

        TagLib::Ogg::Vorbis::File vorbis_file(gd_string_to_filename(audio_filepath));
        if (!vorbis_file.isValid()) { return fields; };

        TagLib::Ogg::XiphComment* vorbis_tag = vorbis_file.tag();
        if (vorbis_tag == NULL) { return fields; };

        TagLib::PropertyMap properties_map = vorbis_tag->properties();
        TagLib::PropertyMap::ConstIterator it;
        for (it = properties_map.begin(); it != properties_map.end(); ++it) {
            fields[it->first.toCString(true)] = it->second.front().toCString(true);
        }

        return fields;
    }


    godot::String get_single_field_property(godot::String audio_filepath, godot::String field_identifiers) {
        godot::String field_value = "";

        TagLib::Ogg::Vorbis::File vorbis_file(gd_string_to_filename(audio_filepath));
        if (!vorbis_file.isValid()) { return field_value; };

        TagLib::Ogg::XiphComment* vorbis_tag = vorbis_file.tag();
        if (vorbis_tag == NULL) { return field_value; };

        TagLib::StringList field_values = vorbis_tag->properties()[field_identifiers.alloc_c_string()];

        if (field_values.size() > 0) {
            field_value = field_values[0].toCString(true);
        }

        return field_value;
    }


    void remove_tag(godot::String audio_filepath) {
        TagLib::Ogg::Vorbis::File vorbis_file(gd_string_to_filename(audio_filepath));
        if (!vorbis_file.isOpen()) { return; };
        vorbis_file.tag()->removeAllFields();
        vorbis_file.save();
    }
};


class FLAC : public OGG_FLAC {
    GODOT_CLASS(FLAC, OGG_FLAC)

public:
    static void _register_methods() {
        register_method("get_cover", &FLAC::get_cover);
        register_method("get_covers", &FLAC::get_covers);
        register_method("copy_cover", &FLAC::copy_cover);
        register_method("add_cover", &FLAC::add_cover);
        register_method("remove_cover", &FLAC::remove_cover);
        register_method("remove_all_covers", &FLAC::remove_all_covers);
        register_method("get_embedded_cover_count", &FLAC::get_embedded_cover_count);
        register_method("remove_tag", &FLAC::remove_tag);
        register_method("set_property", &FLAC::set_property);
        register_method("get_single_field_property", &FLAC::get_single_field_property);
        register_method("set_field", &FLAC::set_field);
    }

    void _init() {
    }

    godot::PoolByteArray get_cover(godot::String audio_filepath, unsigned int cover_idx) {
        TagLib::FLAC::File flac_file(gd_string_to_filename(audio_filepath));
        if (!flac_file.isOpen()) { return godot::PoolByteArray(); }

        unsigned int counter = 0;
        TagLib::List<TagLib::FLAC::Picture*> pictures = flac_file.pictureList();
        for (TagLib::List<TagLib::FLAC::Picture*>::ConstIterator it = pictures.begin(); it != pictures.end(); it++) {
            if (counter == cover_idx) {
                return ByteVector2PoolByte((*it)->render());
            }
            counter += 1;
        }

        return godot::PoolByteArray();
    }


    godot::Array get_covers(godot::String audio_filepath) {
        TagLib::FLAC::File flac_file(gd_string_to_filename(audio_filepath));
        if (!flac_file.isOpen()) { return godot::Array(); }

        godot::Array cover_data = godot::Array();
        TagLib::List<TagLib::FLAC::Picture*> pictures = flac_file.pictureList();
        for (TagLib::List<TagLib::FLAC::Picture*>::ConstIterator it = pictures.begin(); it != pictures.end(); it++) {
            cover_data.push_back(ByteVector2PoolByte((*it)->data()));
        }
        return cover_data;
    }


    bool add_cover(godot::String audio_filepath, godot::String image_filepath, godot::String mime_type) {

        TagLib::FLAC::File flac_file(gd_string_to_filename(audio_filepath));
        if (!flac_file.isOpen()) { return false; }

        ImageFile image = ImageFile(gd_string_to_filename(image_filepath));
        flac_file.addPicture(create_flac_picture(image.data(), mime_type.alloc_c_string()));

        return flac_file.save();
    }


    bool remove_all_covers(godot::String audio_filepath) {
        TagLib::FLAC::File flac_file(gd_string_to_filename(audio_filepath));
        if (!flac_file.isOpen()) { return false; }

        flac_file.removePictures();
        return flac_file.save();
    }


    bool remove_cover(godot::String audio_filepath, unsigned int cover_idx) {
        TagLib::FLAC::File flac_file(gd_string_to_filename(audio_filepath));
        if (!flac_file.isOpen()) { return false; }

        unsigned int counter = 0;
        TagLib::List<TagLib::FLAC::Picture*> pictures = flac_file.pictureList();
        for (TagLib::List<TagLib::FLAC::Picture*>::ConstIterator it = pictures.begin(); it != pictures.end(); it++) {
            if (counter == cover_idx) {
                flac_file.removePicture(*it, true);
                break;
            }
            counter += 1;
        }

        return flac_file.save();
    }


    godot::Dictionary get_all_fields(godot::String audio_filepath) {
        godot::Dictionary fields = {};

        TagLib::FLAC::File vorbis_file(gd_string_to_filename(audio_filepath));
        if (!vorbis_file.isValid()) { return fields; };

        TagLib::Tag* flac_tag = vorbis_file.tag();
        if (flac_tag == NULL) { return fields; };

        TagLib::PropertyMap properties_map = flac_tag->properties();
        TagLib::PropertyMap::ConstIterator it;
        for (it = properties_map.begin(); it != properties_map.end(); ++it) {
            fields[it->first.toCString(true)] = it->second.front().toCString(true);
        }

        return fields;
    }


    bool set_field(godot::String audio_filepath, godot::String field_id, godot::String field_data) {
        TagLib::FLAC::File vorbis_file(gd_string_to_filename(audio_filepath));
        if (!vorbis_file.isValid()) { return false; };

        TagLib::Tag* flac_tag = vorbis_file.tag();
        if (flac_tag == NULL) { return false; };

        TagLib::PropertyMap new_properties;
        new_properties.insert(field_id.alloc_c_string(), TagLib::StringList(field_data.alloc_c_string()));
        flac_tag->setProperties(new_properties);

        return vorbis_file.save();
    }


    godot::String get_single_field_property(godot::String audio_filepath, godot::String field_identifier) {
        godot::String field_value = "";

        TagLib::FLAC::File flac_file(gd_string_to_filename(audio_filepath));
        if (!flac_file.isValid()) { return field_value; };

        TagLib::Tag* flac_tag = flac_file.tag();
        if (flac_tag == NULL) { return field_value; };

        TagLib::StringList field_values = flac_tag->properties()[field_identifier.alloc_c_string()];

        if (field_values.size() > 0) {
            field_value = field_values[0].toCString(true);
        }

        return field_value;
    }


    bool copy_cover(godot::String audio_filepath, godot::String dst_filepath, unsigned int cover_idx) {
        TagLib::FLAC::File flac_file(gd_string_to_filename(audio_filepath));
        if (!flac_file.isValid()) { return false; }

        unsigned int counter = 0;
        TagLib::List<TagLib::FLAC::Picture*> pictures = flac_file.pictureList();
        for (TagLib::List<TagLib::FLAC::Picture*>::ConstIterator it = pictures.begin(); it != pictures.end(); it++) {
            if (counter == cover_idx) {
                return save_buffer_to_file(dst_filepath, (*it)->data());
            }
            counter += 1;
        }
        return false;
    }


    unsigned int get_embedded_cover_count(godot::String filepath) {
        // returns the amount of embedded covers within file
        TagLib::FLAC::File flac_file(gd_string_to_filename(filepath));
        if (!flac_file.isOpen()) { return 0; };

        return flac_file.pictureList().size();
    }


    void remove_tag(godot::String audio_filepath) {
        TagLib::FLAC::File flac_file(gd_string_to_filename(audio_filepath));
        if (!flac_file.isValid()) { return; }
        flac_file.strip(TagLib::FLAC::File::AllTags);
        flac_file.save();
    }


    bool set_property(godot::String audio_filepath, godot::String field_id, godot::String field_data) {
        TagLib::FLAC::File flac_file(gd_string_to_filename(audio_filepath));
        if (!flac_file.isValid()) { return false; }

        TagLib::Tag* flac_tag = flac_file.tag();
        if (flac_tag == NULL) { return false; };

        flac_file.xiphComment(true)->addField(field_id.alloc_c_string(), field_data.alloc_c_string(), true);

        return flac_file.save();
    }
};


class MP4 : public godot::Reference {
    GODOT_CLASS(MP4, godot::Reference)

public:
    static void _register_methods() {
        register_method("get_cover", &MP4::get_cover);
        register_method("set_item", &MP4::copy_cover);
        register_method("get_covers", &MP4::get_covers);
        register_method("insert_cover", &MP4::insert_cover);
        register_method("add_cover", &MP4::add_cover);
        register_method("get_embedded_cover_count", &MP4::get_embedded_cover_count);
        register_method("remove_tag", &MP4::remove_tag);
        register_method("set_item", &MP4::set_item);
    }

    void _init() {
    }

    godot::PoolByteArray get_cover(godot::String filepath, unsigned int cover_idx) {
        TagLib::MP4::File mp4_file(gd_string_to_filename(filepath));
        if (!mp4_file.isOpen()) { return godot::PoolByteArray(); }

        TagLib::MP4::Tag* mp4_tag = mp4_file.tag();
        if (mp4_tag == NULL) { return godot::PoolByteArray(); }

        unsigned int counter = 0;
        TagLib::MP4::CoverArtList covers = mp4_tag->itemMap()["covr"].toCoverArtList();
        for (TagLib::MP4::CoverArtList::ConstIterator it = covers.begin(); it != covers.end(); it++) {
            if (counter == cover_idx) {
                return ByteVector2PoolByte(it->data());
            }
            counter += 1;
        }
        return godot::PoolByteArray();
    }


    void insert_cover(godot::String dst_path, godot::String src_path, godot::String mime_type, unsigned int cover_idx) {
        TagLib::MP4::File audioFile(gd_string_to_filename(dst_path));
        TagLib::MP4::Tag* mp4_tag = audioFile.tag();
        ImageFile imageFile = ImageFile(gd_string_to_filename(src_path));
        TagLib::MP4::CoverArt coverArt(TagLib::MP4::CoverArt::Format::JPEG, imageFile.data());

        // create cover art list
        TagLib::MP4::CoverArtList coverArtList = mp4_tag->itemMap()["covr"].toCoverArtList();

        unsigned int counter = 0;
        for (TagLib::MP4::CoverArtList::Iterator it = coverArtList.begin(); it != coverArtList.end(); it++) {
            if (counter == cover_idx) {
                it = coverArtList.insert(it, coverArt);
                break;
            }
            counter += 1;
        }

        // add item to map
        mp4_tag->setItem("covr", coverArtList);

        mp4_tag->save();
        audioFile.save();
    }


    bool remove_all_covers(godot::String audio_filepath) {
        TagLib::MP4::File mp4_file(gd_string_to_filename(audio_filepath));
        if (!mp4_file.isOpen()) { return false; }
        TagLib::MP4::Tag* mp4_tag = mp4_file.tag();
        if (mp4_tag->isEmpty()) { return false; }
        TagLib::MP4::ItemMap itemsMap = mp4_tag->itemMap();
        mp4_tag->removeItem("covr");
        mp4_tag->save();
        mp4_file.save();
        return true;
    }


    bool add_cover(godot::String dst_path, godot::String src_path) {
        ImageFile image(gd_string_to_filename(src_path));
        if (!image.isOpen()) {
            godot::Godot::print("ERROR://MP4, Could not open Image");
            return false;
        }
        TagLib::MP4::CoverArt new_coverart(TagLib::MP4::CoverArt::Format::Unknown, image.data());
        TagLib::MP4::File mp4_file(gd_string_to_filename(dst_path));

        TagLib::MP4::Tag* mp4_tag = mp4_file.tag();
        TagLib::MP4::ItemMap itemsListMap = mp4_tag->itemListMap();

        TagLib::MP4::CoverArtList coverArtList = mp4_tag->itemListMap()["covr"].toCoverArtList();
        mp4_tag->setItem("covr", coverArtList.append(new_coverart));

        return mp4_file.save() && mp4_tag->save();
    }


    bool copy_cover(godot::String audio_filepath, godot::String dst_filepath, unsigned int cover_idx) {
        TagLib::MP4::File mp4_file(gd_string_to_filename(audio_filepath));
        if (!mp4_file.isOpen()) { return false; }

        TagLib::MP4::Tag* mp4_tag = mp4_file.tag();
        if (mp4_tag == NULL) { return false; }

        TagLib::MP4::CoverArtList coverArtList = mp4_tag->itemMap()["covr"].toCoverArtList();

        unsigned int counter = 0;
        for (TagLib::MP4::CoverArtList::ConstIterator it = coverArtList.begin(); it != coverArtList.end(); it++) {
            if (counter == cover_idx) {
                return save_buffer_to_file(dst_filepath, it->data());
            }
            counter += 1;
        }
        return false;
    }


    godot::Array get_covers(godot::String filepath) {
        godot::Array cover_data = godot::Array();
        TagLib::MP4::File mp4_file(gd_string_to_filename(filepath));
        if (mp4_file.isValid() && mp4_file.hasMP4Tag()) {
            TagLib::MP4::Tag* mp4_tag = mp4_file.tag();
            if (mp4_tag->isEmpty()) {
                return cover_data;
            }
            TagLib::MP4::CoverArtList covers = mp4_tag->itemMap()["covr"].toCoverArtList();
            for (TagLib::MP4::CoverArtList::ConstIterator it = covers.begin(); it != covers.end(); it++) {
                cover_data.push_back(ByteVector2PoolByte(it->data()));
            }
        }
        return cover_data;
    }

    unsigned int get_embedded_cover_count(godot::String filepath) {
        // returns the amount of embedded covers within an MP4 tagged file
        TagLib::MP4::File mp4_file(gd_string_to_filename(filepath));
        if (!mp4_file.isOpen()) { return 0; }

        TagLib::MP4::Tag* mp4_tag = mp4_file.tag();
        if (mp4_tag == NULL) { return 0; }

        return mp4_tag->itemMap()["covr"].toCoverArtList().size();
    }


    bool set_item(godot::String audio_filepath, godot::String item_key, godot::String item_value) {

        TagLib::MP4::File mp4_file(gd_string_to_filename(audio_filepath));
        if (!mp4_file.isOpen()) { return false; }

        TagLib::MP4::Tag* mp4_tag = mp4_file.tag();
        if (mp4_tag == NULL) { return false; }

        const char* mp4_metadata_keys[] = {
            "©alb",
            "©art",
            "aART",
            "©cmt",
            "©nam",
            "©ART",
            "aART",
            "©alb",
            "©grp",
            "©wrt",
            "©cmt",
            "gnre",
            "©gen",
            "©day",
            "trkn",
            "disk",
            "tmpo",
            "cpil",
            "tvsh",
            "tven",
            "tvsn",
            "tves",
            "tvnn",
            "desc",
            "ldes",
            "©lyr",
            "sonm",
            "soar",
            "soaa",
            "soal",
            "soco",
            "sosn",
            "covr",
            "cprt",
            "©too",
            "©enc",
            "purd",
            "pcst",
            "purl",
            "keyw",
            "catg",
            "hdvd",
            "stik",
            "rtng",
            "pgap",
            "apID",
            "akID",
            "cnID",
            "sfID",
            "atID",
            "plID",
            "geID",
            "©st3",
        };


        godot::Godot::print(item_key);
        godot::Godot::print(item_value);

        mp4_tag->setItem(item_key.alloc_c_string(), TagLib::StringList(item_value.alloc_c_string()));

        return mp4_file.save();
    }

    godot::String get_single_item(godot::String audio_filepath, godot::String item_identifier) {
        godot::String item_value = "";

        TagLib::MP4::File mp4_file(gd_string_to_filename(audio_filepath));
        if (!mp4_file.isOpen()) { return item_value; }

        TagLib::MP4::Tag* mp4_tag = mp4_file.tag();
        if (mp4_tag == NULL) { return item_value; }

        TagLib::StringList item_values = mp4_tag->itemListMap()[item_identifier.alloc_c_string()].toStringList();

        if (item_values.size() > 0) {
            item_value = item_values[0].toCString(true);
        }

        return item_value;
    }


    godot::Dictionary get_all_items(godot::String audio_filepath) {
        godot::Dictionary items;

        TagLib::MP4::File mp4_file(gd_string_to_filename(audio_filepath));
        if (!mp4_file.isValid()) {
            // File is not valid, return empty dictionary
            return items;
        }

        TagLib::MP4::Tag* mp4_tag = mp4_file.tag();
        if (mp4_tag == NULL) {
            // No tag found in the file, return empty dictionary
            return items;
        }

        TagLib::MP4::ItemMap item_map = mp4_tag->itemMap();
        TagLib::StringList temp_item;

        for (auto it = item_map.begin(); it != item_map.end(); it++) {
            // Get Frame Data
            temp_item = it->second.toStringList();
            // Append string representation of frame if it exists
            if (temp_item.size() > 0) {
                if (it->first.startsWith("©")) {
                    items[it->first.substr(1, -1).toCString(true)] = temp_item[0].toCString(true);
                }
                else {
                    items[it->first.toCString(true)] = temp_item[0].toCString(true);
                }
            }
        }

        return items;
    }


    void remove_tag(godot::String filepath) {
        TagLib::MP4::File mp4_file(gd_string_to_filename(filepath));
        if (!mp4_file.isValid()) { return; }
        mp4_file.strip(TagLib::MP4::File::AllTags);
        mp4_file.save();
    }
};


class Tagging : public godot::Reference {
    GODOT_CLASS(Tagging, godot::Reference)

        godot::String get_file_extension(godot::String filepath) {
        return filepath.get_extension().to_upper();
    }

    enum {
        ARTIST,
        TITLE,
        ALBUM,
        GENRE,
        COMMENT,
        YEAR,
        TRACK
    };

public:
    static void _register_methods() {
        register_method("get_multiple_tags", &Tagging::get_multiple_tags);
        register_method("get_multiple_text_tags", &Tagging::get_multiple_text_tags);
        register_method("get_single_tag", &Tagging::get_single_tag);
        register_method("copy_covers", &Tagging::copy_covers);
        register_method("copy_cover", &Tagging::copy_cover);
        register_method("set_tag", &Tagging::set_tag);
        register_method("add_cover", &Tagging::add_cover);
        register_method("remove_cover", &Tagging::remove_cover);
        register_method("remove_tag", &Tagging::remove_tag);
        register_method("remove_all_covers", &Tagging::remove_all_covers);
        register_method("get_embedded_cover", &Tagging::get_embedded_cover);
        register_method("get_embedded_covers", &Tagging::get_embedded_covers);
        register_method("set_lyrics", &Tagging::set_lyrics);
        register_method("get_lyrics", &Tagging::get_lyrics);
        register_method("get_cover_description", &Tagging::get_cover_description);
        register_method("set_cover_description", &Tagging::set_cover_description);
        register_method("get_embedded_cover_count", &Tagging::get_embedded_cover_count);
        register_method("get_song_popularity", &Tagging::get_song_popularity);
        register_method("set_song_popularity", &Tagging::set_song_popularity);
        register_method("add_property", &Tagging::add_property);
        register_method("export_all_embedded_covers", &Tagging::export_all_embedded_covers);
        register_method("get_text_properties", &Tagging::get_text_properties);
        register_method("get_single_text_property", &Tagging::get_single_text_property);
        register_method("clear_text_property", &Tagging::clear_text_property);
        register_method("remove_text_property", &Tagging::remove_text_property);
        register_method("set_text_properties", &Tagging::set_text_properties);
        register_method("get_image_properties", &Tagging::get_image_properties);
        register_method("get_property_identifiers", &Tagging::get_property_identifiers);
    }

    void _init() {
    }

    godot::PoolStringArray get_multiple_tags(godot::String path, godot::PoolIntArray tagmap) {
        return get_text_tags_single(path, tagmap);
    }

    godot::Array get_multiple_text_tags(godot::PoolStringArray paths, godot::PoolIntArray tagmap) {
        godot::Array text_tags = godot::Array();
        for (size_t i = 0; i < paths.size(); i++) {
            text_tags.push_back(get_text_tags_single(paths[i], tagmap));
        }
        return text_tags;
    }

    godot::PoolByteArray copy_covers(godot::PoolStringArray src_paths, godot::PoolStringArray dst_paths, unsigned int cover_idx) {
        //TagLib::ByteVector TempByteData;
        godot::PoolByteArray return_values;
        for (size_t i = 0; i < src_paths.size(); i++) {
            return_values.push_back(copy_cover(src_paths[i], dst_paths[i], cover_idx));
        }
        return return_values;
    }

    godot::String get_single_tag(godot::String path, unsigned int tagflag) {
        godot::PoolIntArray x = godot::PoolIntArray();
        x.push_back(tagflag);
        godot::Godot::print(path);
        godot::PoolStringArray data = get_text_tags_single(path, x);
        godot::Godot::print(path);
        if (data.size() == 0) { return ""; }
        return data[0];
    }

    bool set_tag(unsigned int flag, godot::String tagdata, godot::String path) {

        TagLib::FileRef file(path.alloc_c_string());
        if (file.isNull()) { return false; }

        TagLib::String data = tagdata.alloc_c_string();

        switch (flag) {
        case ARTIST:
            file.tag()->setArtist(data);
            break;
        case TITLE:
            file.tag()->setTitle(data);
            break;
        case ALBUM:
            file.tag()->setAlbum(data);
            break;
        case GENRE:
            file.tag()->setGenre(data);
            break;
        case COMMENT:
            file.tag()->setComment(data);
            break;
        case YEAR:
            file.tag()->setYear(std::stoi(&data[0]));
            break;
        case TRACK:
            file.tag()->setTrack(std::stoi(&data[0]));
            break;
        default:
            //Invalid Flag given -> no Tag was set
            break;
        }

        //saving all changes made to the file
        return file.save();
    }

    godot::PoolStringArray get_text_tags_single(godot::String path, godot::PoolIntArray type) {
        // This function receives a Path as std:: String and a PoolInt Array
        // The type array hast the indexes of the switch statement
        // This is done to be able to return multiple tags at once
        TagLib::FileRef f(gd_string_to_filename(path));
        godot::PoolStringArray text_tags = godot::PoolStringArray();
        text_tags.resize(type.size());
        if (!f.isNull()) {
            TagLib::String tagdata;
            for (size_t i = 0; i < type.size(); i++) {
                switch (type[static_cast<unsigned int>(i)]) {
                case ARTIST:
                    tagdata = f.tag()->artist();
                    break;
                case TITLE:
                    tagdata = f.tag()->title();
                    break;
                case ALBUM:
                    tagdata = f.tag()->album();
                    break;
                case GENRE:
                    tagdata = f.tag()->genre();
                    break;
                case COMMENT:
                    tagdata = f.tag()->comment();
                    break;
                case YEAR:
                    tagdata = std::to_string(f.tag()->year());
                    break;
                case TRACK:
                    tagdata = std::to_string(f.tag()->track());
                    break;
                }
                text_tags.set(i, tagdata.toCString(true));
            }
        }
        return text_tags;
    }


    godot::String get_cover_description(godot::String path) {
        TagLib::String filetype = get_extension(path.alloc_c_string());
        if (filetype == "MP3" || filetype == "WAV") {
            TagLib::MPEG::File mp3File(gd_string_to_filename(path));
            TagLib::ID3v2::Tag* mp3Tag;
            TagLib::ID3v2::FrameList listOfMp3Frames;
            TagLib::ID3v2::AttachedPictureFrame* pictureFrame;
            if (mp3File.isOpen()) {
                if (!mp3File.hasID3v2Tag()) {
                    return "";
                }
                mp3Tag = mp3File.ID3v2Tag();
                if (mp3Tag != NULL)
                {
                    listOfMp3Frames = mp3Tag->frameListMap()["APIC"];//look for picture frames only
                    if (listOfMp3Frames.isEmpty()) {
                        return "";
                    }
                    pictureFrame = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(listOfMp3Frames[0]);
                    return (pictureFrame->description()).toCString(true);
                }
            }
        }
        else if (filetype == "OGG") {
            TagLib::Ogg::Vorbis::File oggFile(gd_string_to_filename(path));
            TagLib::Ogg::XiphComment* tag = oggFile.tag();
            TagLib::List<TagLib::FLAC::Picture*> Pictures = tag->pictureList();
            return (Pictures[0]->description()).toCString(true);
        }
        return "";
    }


    void set_cover_description(godot::String path, godot::String NewCoverDescription) {
        TagLib::String filetype = get_extension(path.alloc_c_string());
        if (filetype == "MP3" || filetype == "WAV") {

        }
        else if (filetype == "OGG") {
            TagLib::Ogg::Vorbis::File oggFile(gd_string_to_filename(path));
            TagLib::Ogg::XiphComment* tag = oggFile.tag();
            TagLib::List<TagLib::FLAC::Picture*> Pictures = tag->pictureList();
            Pictures[0]->setDescription(NewCoverDescription.alloc_c_string());
            oggFile.save();
        }
    }


    bool export_all_embedded_covers(godot::String audio_filepath, godot::String dst_filepath) {
        godot::String filetype = get_file_extension(audio_filepath);
        if (filetype == "MP3" || filetype == "WAV") {
            return MPEG().export_embedded_covers(audio_filepath, dst_filepath);
        }
        else if (filetype == "OGG") {
            //return OGG_VORBIS().copy_cover(audio_filepath, dst_filepath);
        }
        else if (filetype == "FLAC") {
            //return FLAC().copy_cover(audio_filepath, dst_filepath);
        }
        // Invalid file extension
        return false;
    }


    bool copy_cover(godot::String audio_filepath, godot::String dst_filepath, unsigned int cover_idx = 0) {
        godot::String filetype = get_file_extension(audio_filepath);

        if (filetype == "MP3" || filetype == "WAV") {
            return MPEG().copy_cover(audio_filepath, dst_filepath, cover_idx);
        }
        else if (filetype == "OGG") {
            return OGG_VORBIS().copy_cover(audio_filepath, dst_filepath, cover_idx);
        }
        else if (filetype == "FLAC") {
            return FLAC().copy_cover(audio_filepath, dst_filepath, cover_idx);
        }
        else if (filetype == "MP4") {
            return MP4().copy_cover(audio_filepath, dst_filepath, cover_idx);
        }
        else {
            // Invalid file extension
            return false;
        }
    }


    bool add_cover(godot::String audio_filepath, godot::String image_path, godot::String mime_type) {
        // Appends the Image into the File without changing the others
        godot::String filetype = get_file_extension(audio_filepath);

        if (filetype == "M4A")
        {
            return MP4().add_cover(audio_filepath, image_path);
        }
        else if (filetype == "MP3" || filetype == "WAV")
        {
            return MPEG().add_cover(audio_filepath, image_path, mime_type);
        }
        else if (filetype == "OGG") {
            return OGG_VORBIS().add_cover(audio_filepath, image_path, mime_type);
        }
        else if (filetype == "FLAC") {
            return FLAC().add_cover(audio_filepath, image_path, mime_type);
        }
        else
        {
            godot::Godot::print("Invalid File Extension");
            return false;
        }
        return true;
    }


    bool remove_cover(godot::String audio_filepath, unsigned int cover_idx) {
        // Appends the Image into the File without changing the others
        godot::String filetype = get_file_extension(audio_filepath);

        if (filetype == "M4A")
        {
            // read the mp4 file
            TagLib::MP4::File audioFile(audio_filepath.alloc_c_string());

            // get the tag ptr
            TagLib::MP4::Tag* tag = audioFile.tag();

            // get the items map
            TagLib::MP4::ItemMap itemsListMap = tag->itemMap();

            // create cover art list
            TagLib::MP4::CoverArtList coverArtList;

            // append instance
            coverArtList = itemsListMap.value("covr").toCoverArtList();

            unsigned int counter = 0;
            for (TagLib::MP4::CoverArtList::Iterator it = coverArtList.begin(); it != coverArtList.end(); it++) {
                if (counter == cover_idx) {
                    coverArtList.erase(it);
                    break;
                }
                counter += 1;
            }

            // updating items
            tag->setItem("covr", coverArtList);

            tag->save();
            audioFile.save();
        }
        else if (filetype == "MP3" || filetype == "WAV") {
            return MPEG().remove_cover(audio_filepath, cover_idx);
        }
        else if (filetype == "OGG") {
            return OGG_VORBIS().remove_cover(audio_filepath, cover_idx);
        }
        else if (filetype == "FLAC") {
            return FLAC().remove_cover(audio_filepath, cover_idx);
        }
        else {
            godot::Godot::print("Invalid File Extension");
            return false;
        }
        return true;
    }


    void remove_text_property(godot::String audio_filepath, godot::String property_identifier) {
        // The property_identifier is always the ogg one, since it is more descriptive for users

        godot::String filetype = get_file_extension(audio_filepath);

        if (filetype == "MP3" || filetype == "WAV") {
            MPEG().remove_text_frame(audio_filepath, field_identifiers[property_identifier.alloc_c_string()].c_str());
        }
        else if (filetype == "OGG" || filetype == "FLAC") {
            OGG_VORBIS().get_all_fields(audio_filepath);
        }
        else if (filetype == "MP4" || filetype == "M4A") {
            MP4().get_all_items(audio_filepath);
        }
    }


    void clear_text_property(godot::String audio_filepath, godot::String property_identifier) {
        // The property_identifier is expected to be in the right format depending on the file format

        godot::String filetype = get_file_extension(audio_filepath);

        if (filetype == "MP3" || filetype == "WAV") {

            MPEG().set_text_frames(audio_filepath, godot::PoolStringArray(), godot::PoolStringArray());
        }
        else if (filetype == "OGG" || filetype == "FLAC") {
            OGG_VORBIS().set_field(audio_filepath, property_identifier, "");
        }
        else if (filetype == "MP4" || filetype == "M4A") {
            MP4().set_item(audio_filepath, property_identifier, "");
        }
    }


    bool remove_all_covers(godot::String audio_filepath) {
        // Appends the Image into the File without changing the others
        godot::String filetype = get_file_extension(audio_filepath);

        if (filetype == "M4A")
        {
            return MP4().remove_all_covers(audio_filepath);
        }
        else if (filetype == "MP3" || filetype == "WAV")
        {
            return MPEG().remove_all_covers(audio_filepath);
        }
        else if (filetype == "OGG") {
            return OGG_VORBIS().remove_all_covers(audio_filepath);
        }
        else if (filetype == "FLAC") {
            return FLAC().remove_all_covers(audio_filepath);
        }
        else
        {
            godot::Godot::print("Invalid File Extension");
            return false;
        }
        return true;
    }


    godot::PoolByteArray get_embedded_cover(godot::String audio_filepath, unsigned int cover_idx) {

        godot::String filetype = get_file_extension(audio_filepath);

        if (filetype == "MP3" || filetype == "WAV") {
            return MPEG().get_cover(audio_filepath, cover_idx);
        }
        else if (filetype == "OGG") {
            return OGG_VORBIS().get_cover(audio_filepath, cover_idx);
        }
        else if (filetype == "FLAC") {
            return FLAC().get_cover(audio_filepath, cover_idx);
        }
        else if (filetype == "M4A" || filetype == "MP4") {
            return MP4().get_cover(audio_filepath, cover_idx);
        }
        return godot::PoolByteArray();
    }


    godot::Array get_embedded_covers(godot::String audio_filepath) {
        // Returns the Data of every Cover embededded in the File given
        // As godot::Array where each index is a godot::PoolByteArray
        godot::String filetype = get_file_extension(audio_filepath);
        if (filetype == "MP3" || filetype == "WAV") {
            return MPEG().get_covers(audio_filepath);
        }
        else if (filetype == "OGG") {
            return OGG_VORBIS().get_covers(audio_filepath);
        }
        else if (filetype == "FLAC") {
            return FLAC().get_covers(audio_filepath);
        }
        else if (filetype == "M4A" || filetype == "MP4") {
            return MP4().get_covers(audio_filepath);
        }
        return godot::Array();
    }


    unsigned int get_embedded_cover_count(godot::String audio_filepath) {

        godot::String filetype = get_file_extension(audio_filepath);

        if (filetype == "MP3" || filetype == "WAV") {
            return MPEG().get_embedded_cover_count(audio_filepath);
        }
        else if (filetype == "OGG") {
            return OGG_VORBIS().get_embedded_cover_count(audio_filepath);
        }
        else if (filetype == "FLAC") {
            return FLAC().get_embedded_cover_count(audio_filepath);
        }
        else if (filetype == "M4A" || filetype == "MP4") {
            return MP4().get_embedded_cover_count(audio_filepath);
        }
        return 0;
    }


    void remove_tag(godot::String audio_filepath) {
        godot::String filetype = get_file_extension(audio_filepath);
        if (filetype == "MP3" || filetype == "WAV") {
            MPEG().remove_tag(audio_filepath);
        }
        else if (filetype == "OGG") {
            OGG_VORBIS().remove_tag(audio_filepath);
        }
        else if (filetype == "FLAC") {
            FLAC().remove_tag(audio_filepath);
        }
        else if (filetype == "M4A" || filetype == "MP4") {
            MP4().remove_tag(audio_filepath);
        }
    }


    void set_lyrics(godot::String audio_filepath, godot::PoolStringArray verses, godot::PoolIntArray timestamps, bool IsSynchronized) {
        std::string FrameTitle = "";
        if (IsSynchronized) {
            //[#sec4.10 Synchronized lyric/text]
            FrameTitle = "SYLT";
        }
        else {
            //[#sec4.9 Unsychronized lyric/text transcription]
            FrameTitle = "USLT";
        }

        TagLib::MPEG::File mp3File(gd_string_to_filename(audio_filepath));

        TagLib::ID3v2::Tag* mp3Tag;

        mp3Tag = mp3File.ID3v2Tag(true);

        TagLib::ID3v2::FrameList listOfMp3Frames;
        if (!mp3File.isOpen()) {
            return;
        }

        listOfMp3Frames = mp3Tag->frameListMap()[FrameTitle.c_str()];
        for (size_t i = 0; i < listOfMp3Frames.size(); i++) {
            mp3Tag->removeFrame(listOfMp3Frames[i]);
        }


        if (IsSynchronized) {

            TagLib::ID3v2::SynchronizedLyricsFrame* NewSynchedLyricsFrame = new TagLib::ID3v2::SynchronizedLyricsFrame;
            TagLib::List< TagLib::ID3v2::SynchronizedLyricsFrame::SynchedText > SynchedText;



            for (size_t i = 0; i < verses.size(); i++) {
                TagLib::ID3v2::SynchronizedLyricsFrame::SynchedText SynchedVerse = {
                    static_cast<unsigned int> (timestamps[i]),
                    verses[i].alloc_c_string(),
                };
                SynchedText.append(SynchedVerse);
            }

            NewSynchedLyricsFrame->setTimestampFormat(NewSynchedLyricsFrame->AbsoluteMilliseconds);
            NewSynchedLyricsFrame->setSynchedText(SynchedText);

            mp3Tag->addFrame(NewSynchedLyricsFrame);
        }
        else {

            TagLib::ID3v2::UnsynchronizedLyricsFrame* NewUnsynchedLyricsFrame = new TagLib::ID3v2::UnsynchronizedLyricsFrame();
            std::string Lyrics = "";
            for (size_t i = 0; i < verses.size(); i++) {
                Lyrics.append(verses[i].alloc_c_string());
                Lyrics.append("\n");
            }
            NewUnsynchedLyricsFrame->setText(Lyrics);
            mp3Tag->addFrame(NewUnsynchedLyricsFrame);
        }
        mp3File.save();
    }


    godot::Array get_lyrics(godot::String audio_filepath) {
        //Retrieving the Lyrics from an Audiofile
        //Synchronized Lyrics will be preferred to Unsynchronized ones
        TagLib::MPEG::File file(gd_string_to_filename(audio_filepath));
        if (!file.isOpen()) { return godot::Array(); }
        if (!file.hasID3v2Tag()) { return godot::Array(); }

        TagLib::ID3v2::Tag* ID3Tag = file.ID3v2Tag();
        TagLib::ID3v2::FrameList synched_frames = ID3Tag->frameListMap()["SYLT"];
        if (!synched_frames.isEmpty()) {
            TagLib::ID3v2::SynchronizedLyricsFrame* SynchedFrame = static_cast<TagLib::ID3v2::SynchronizedLyricsFrame*> (synched_frames[0]);
            godot::PoolStringArray synched_text;
            godot::PoolRealArray synched_timestamps;
            TagLib::List< TagLib::ID3v2::SynchronizedLyricsFrame::SynchedText > x = SynchedFrame->synchedText();
            //SynchedFrame->timestampFormat();


            for (size_t i = 0; i < x.size(); i++) {
                synched_text.push_back(x[i].text.toCString(true));
                TagLib::ID3v2::SynchronizedLyricsFrame* SynchedFrame = static_cast<TagLib::ID3v2::SynchronizedLyricsFrame*> (synched_frames[0]);
                switch (SynchedFrame->timestampFormat()) {
                case 2:             //Absolute Milliseconds
                    synched_timestamps.push_back(static_cast<float>(x[i].time) / 1000.0);
                    break;
                case 1:             //Absolute MPEG Frames
                {
                    //Converting the Absolute MPEG Frames to Seconds
                    unsigned int sample_rate = file.audioProperties()->sampleRate();
                    unsigned int samples_per_frame = 0;
                    unsigned int abs_mpeg_frames = x[i].time;
                    if (sample_rate >= 16000 && sample_rate <= 24000) {
                        samples_per_frame = 576;
                    }
                    else if (sample_rate >= 32000 && sample_rate <= 48000) {
                        samples_per_frame = 1152;
                    }
                    synched_timestamps.push_back(1000.0 / (file.audioProperties()->sampleRate()) * samples_per_frame * abs_mpeg_frames / 1000.0);
                    break;
                }
                case 0:             //Unknown
                    //Trying to interpret them as seconds, beacause it's the most common format
                    synched_timestamps.push_back(static_cast<float>(x[i].time) / 1000.0);
                    break;
                }
            }

            godot::Array synched_lyrics;

            synched_lyrics.push_back(synched_text);
            synched_lyrics.push_back(synched_timestamps);

            //Array -> [Lyrics, TimeStamps]
            return synched_lyrics;
            //If there is a Synchrnozed Lyrics Frame
        }
        else {
            synched_frames = ID3Tag->frameListMap()["USLT"];
            if (!synched_frames.isEmpty()) {
                //if there is an unsynchronized Lyrics Frame
                TagLib::ID3v2::UnsynchronizedLyricsFrame* unsynched_frame = static_cast<TagLib::ID3v2::UnsynchronizedLyricsFrame*> (synched_frames[0]);
                godot::Array unsynched_text;
                unsynched_text.push_back(unsynched_frame->text().toCString(true));
                return unsynched_text;
            }
        }
        return godot::Array();
    }


    void set_song_popularity(godot::String audio_filepath, int rating_out_of_10 = -1, int Counter = -1, godot::String Email = "") {
        godot::String filetype = get_file_extension(audio_filepath);
        if (filetype == "MP3" || filetype == "WAV") {
            godot::PoolStringArray frame_data = get_song_popularity(audio_filepath);
            TagLib::MPEG::File mpeg_file(gd_string_to_filename(audio_filepath));
            if (!mpeg_file.isValid() || !mpeg_file.hasID3v2Tag()) { return; }
            TagLib::ID3v2::Tag* mpeg_tag = mpeg_file.ID3v2Tag();
            if (mpeg_tag == NULL) { return; }
            TagLib::ID3v2::FrameList RatingFrames = mpeg_tag->frameListMap()["POPM"];
            TagLib::ID3v2::PopularimeterFrame* rating_frame = new TagLib::ID3v2::PopularimeterFrame();
            if (RatingFrames.size() > 0) {
                mpeg_tag->removeFrames("POPM");
                rating_frame->setRating(std::stoi(frame_data[0].alloc_c_string()));
                rating_frame->setCounter(std::stoi(frame_data[1].alloc_c_string()));
                rating_frame->setEmail(frame_data[1].alloc_c_string());
            }

            // filling in frame data
            if (Email != "") { rating_frame->setEmail(Email.alloc_c_string()); }
            if (rating_out_of_10 != -1) { rating_frame->setRating(rating_out_of_10); }
            if (Counter != -1) { rating_frame->setCounter(Counter); }

            mpeg_tag->addFrame(static_cast<TagLib::ID3v2::Frame*>(rating_frame));
            mpeg_file.save();
            return;
        }
        else if (filetype == "OGG") {
            //Adding using custom fields -> non standarddized way
            TagLib::Ogg::Vorbis::File vorbis_file(gd_string_to_filename(audio_filepath));
            TagLib::Ogg::XiphComment* vorbis_tag = vorbis_file.tag();
            if (rating_out_of_10 != -1) { vorbis_tag->addField("Rating", std::to_string(rating_out_of_10), true); }
            if (Counter != -1) { vorbis_tag->addField("Counter", std::to_string(Counter), true); }
            if (Email != "") { vorbis_tag->addField("Email", Email.alloc_c_string(), true); }
            vorbis_file.save();
        }
    }


    godot::Array get_song_popularity(godot::String audio_filepath) {
        godot::Array rating_data;
        rating_data.resize(3);

        godot::String filetype = get_file_extension(audio_filepath);

        if (filetype == "MP3" || filetype == "WAV") {
            // reading the rating from the frame Popularitymeter POPM if existing
            TagLib::MPEG::File mpeg_file(gd_string_to_filename(audio_filepath));
            if (!mpeg_file.isValid() || !mpeg_file.hasID3v2Tag()) { return rating_data; }
            TagLib::ID3v2::Tag* ID3Tag = mpeg_file.ID3v2Tag();
            TagLib::ID3v2::FrameList RatingFrames = ID3Tag->frameListMap()["POPM"];
            if (RatingFrames.size() > 0) {
                TagLib::ID3v2::PopularimeterFrame* RatingFrame = static_cast<TagLib::ID3v2::PopularimeterFrame*>(RatingFrames[0]);
                rating_data[0] = RatingFrame->rating();
                rating_data[1] = RatingFrame->counter();
                rating_data[2] = RatingFrame->email().toCString(true);
            }
        }
        else if (filetype == "OGG") {
            // reading the rating from custom fields -> Rating, Counter, Email
            TagLib::Ogg::Vorbis::File ogg_file(gd_string_to_filename(audio_filepath));
            if (!ogg_file.isOpen()) { return rating_data; }
            TagLib::Ogg::XiphComment* OggTag = ogg_file.tag();
            TagLib::PropertyMap fields = OggTag->properties();
            if (fields.contains("Rating")) {
                rating_data[0] = (fields["Rating"][0]).toCString(true);
            }
            if (fields.contains("Counter")) {
                rating_data[1] = (fields["Counter"][0]).toCString(true);
            }
            if (fields.contains("Email")) {
                rating_data[2] = (fields["Email"][0]).toCString(true);
            }
        }
        return rating_data;
    }


    bool add_property(godot::String audio_filepath, godot::String field_id, godot::String field_data) {
        godot::String filetype = get_file_extension(audio_filepath);

        if (filetype == "OGG") {
            return OGG_VORBIS().set_field(audio_filepath, field_id, field_data);
        }
        else if (filetype == "FLAC") {
            return FLAC().set_property(audio_filepath, field_id, field_data);
        }
        else if (filetype == "MP3" || filetype == "WAV") {
            godot::PoolStringArray id;
            id.append(field_id);
            godot::PoolStringArray data;
            data.append(field_data);
            return MPEG().set_text_frames(audio_filepath, id, data);
        }
        else if (filetype == "MP4" || filetype == "M4A") {
            return MP4().set_item(audio_filepath, field_id, field_data);
        }
        return false;
    }


    godot::Dictionary get_text_properties(godot::String audio_filepath) {
        godot::String filetype = get_file_extension(audio_filepath);

        if (filetype == "MP3" || filetype == "WAV") {
            return MPEG().get_text_properties(audio_filepath);
        }
        else if (filetype == "OGG") {
            return OGG_VORBIS().get_all_fields(audio_filepath);
        }
        else if (filetype == "FLAC") {
            return FLAC().get_all_fields(audio_filepath);
        }
        else if (filetype == "MP4" || filetype == "M4A") {
            // The Copyright Symbol will be cut from the MP$ Item Identifier, since GDNative cannot handle it
            // -> will be readded within godot project (can handle it)
            return MP4().get_all_items(audio_filepath);
        }

        return godot::Dictionary();
    }


    godot::Dictionary get_image_properties(godot::String filepath, unsigned int cover_idx) {
        godot::Dictionary properties = godot::Dictionary();

        godot::PoolByteArray image_data = get_embedded_cover(filepath, cover_idx);
        if (image_data.size() < 128) { return {}; }
        godot::PoolByteArray image_header = godot::PoolByteArray();
        image_header.resize(128);

        for (size_t i = 0; i < 128; i++) {
            image_header.set(i, image_data[i]);
        }

        properties["size"] = image_data.size();
        properties["mime_type"] = get_mime_type(image_header);
        properties["dimensions"] = godot::Vector2();
        properties["image_type"] = get_cover_type(filepath, cover_idx);

        return properties;
    }

    godot::String get_cover_type(godot::String audio_filepath, unsigned int cover_idx) {
        std::string image_types[] = {
            "Other",
            "FileIcon",
            "OtherFileIcon",
            "FrontCover",
            "BackCover",
            "LeafletPage",
            "Media",
            "LeadArtist",
            "Artist",
            "Conductor",
            "Band",
            "Composer",
            "Lyricist",
            "RecordingLocation",
            "DuringRecording",
            "DuringPerformance",
            "MovieScreenCapture",
            "ColouredFish",
            "Illustration",
            "BandLogo",
            "PublisherLogo"
        };

        godot::String filetype = get_file_extension(audio_filepath);

        if (filetype == "MP3" || filetype == "WAV") {
            return image_types[MPEG().get_cover_type(audio_filepath, cover_idx)].c_str();
        }
        else if (filetype == "OGG" || filetype == "FLAC") {
            //return OGG_VORBIS().get_cover_type(audio_filepath);
        }
        else if (filetype == "MP4" || filetype == "M4A") {
            //return MP4().get_cover_type(audio_filepath);
        }

        return image_types[0].c_str();
    }

    void set_text_properties(godot::String audio_filepath, godot::PoolStringArray identifiers, godot::PoolStringArray values) {
        godot::String filetype = get_file_extension(audio_filepath);
        if (filetype == "MP3" || filetype == "WAV") {
            MPEG().set_text_frames(audio_filepath, identifiers, values);
        }
        else if (filetype == "OGG") {
            for (size_t i = 0; i < identifiers.size(); i++) {
                OGG_VORBIS().set_field(audio_filepath, identifiers[i], values[i]);
            }
        }
        else if (filetype == "FLAC") {
            for (size_t i = 0; i < identifiers.size(); i++) {
                FLAC().set_field(audio_filepath, identifiers[i], values[i]);
            }
        }
        else if (filetype == "M4A" || filetype == "MP4") {
            for (size_t i = 0; i < identifiers.size(); i++) {
                MP4().set_item(audio_filepath, identifiers[i], values[i]);
            }
        }
    }

    godot::PoolStringArray get_property_identifiers(godot::String audio_filepath) {
        godot::String filetype = get_file_extension(audio_filepath);
        if (filetype == "MP3" || filetype == "WAV") {
            return MPEG().get_text_frame_ids(audio_filepath);
        }
        else {
            godot::PoolStringArray field_ids = godot::PoolStringArray();
            for (auto it = field_identifiers.begin(); it != field_identifiers.end(); it++) {
                field_ids.push_back(it->first.c_str());
            }
            return field_ids;
        }
        return godot::PoolStringArray();
    }

    godot::String get_single_text_property(godot::String audio_filepath, godot::String identifier) {
        godot::String filetype = get_file_extension(audio_filepath);
        if (filetype == "MP3" || filetype == "WAV") {
            return MPEG().get_single_text_frame(audio_filepath, identifier);
        }
        else if (filetype == "OGG") {
            return OGG_VORBIS().get_single_field_property(audio_filepath, identifier);
        }
        else if (filetype == "FLAC") {
            return FLAC().get_single_field_property(audio_filepath, identifier);
        }
        else if (filetype == "M4A") {
            return MP4().get_single_item(audio_filepath, identifier);
        }
        return "";
    }
};

extern "C" void GDN_EXPORT godot_gdnative_init(godot_gdnative_init_options * o)
{
    godot::Godot::gdnative_init(o);
}

extern "C" void GDN_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options * o)
{
    godot::Godot::gdnative_terminate(o);
}

extern "C" void GDN_EXPORT godot_nativescript_init(void* handle)
{
    godot::Godot::nativescript_init(handle);
    godot::register_class<Tagging>();
    godot::register_class<AudioProperties>();
    godot::register_class<MPEG>();
    godot::register_class<MP4>();
    godot::register_class<OGG_FLAC>();
    godot::register_class<FLAC>();
    godot::register_class<OGG_VORBIS>();
}