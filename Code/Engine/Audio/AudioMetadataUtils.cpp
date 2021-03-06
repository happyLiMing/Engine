#include "Engine/Audio/AudioMetadataUtils.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "ThirdParty/taglib/include/taglib/mpegfile.h"
#include "ThirdParty/taglib/include/taglib/id3v2tag.h"
#include "ThirdParty/taglib/include/taglib/attachedpictureframe.h"
#include "ThirdParty/taglib/include/taglib/flacfile.h"
#include "ThirdParty/taglib/include/taglib/unknownframe.h"
#include "ThirdParty/taglib/include/taglib/tfile.h"
#include "ThirdParty/taglib/include/taglib/tpropertymap.h"
#include "ThirdParty/taglib/include/taglib/fileref.h"
#include "ThirdParty/taglib/include/taglib/wavfile.h"
#include "ThirdParty/taglib/include/taglib/rifffile.h"
#include "ThirdParty/taglib/include/taglib/oggflacfile.h"
#include "Engine/Input/Console.hpp"
#include <string>

//-----------------------------------------------------------------------------------
std::string GetFileExtension(const std::string& fileName)
{
    //Find the file extension.
    //Use fileName.rfind to find the first period, marking the extension.
    //Get the rest of the characters after the period and return a lowercase string.

    unsigned extensionPos = fileName.rfind('.');

    if (extensionPos != std::string::npos && extensionPos != fileName.length())
    {
        std::string fileExtension = fileName.substr((extensionPos + 1), fileName.length());
        char* fileExtensionChar = new char[fileExtension.length() + 1];
        strcpy(fileExtensionChar, fileExtension.c_str());
        for (unsigned i = 0; i < fileExtension.length(); ++i)
        {
            if (!islower(fileExtensionChar[i]))
            {
                fileExtensionChar[i] = (char)tolower(fileExtensionChar[i]);
            }
        }
        std::string lowerFileExtension(fileExtensionChar);
        delete[] fileExtensionChar;

        return lowerFileExtension;
    }

    return "ERROR";
}

//-----------------------------------------------------------------------------------
std::string GetFileName(const std::string& filePath)
{
    //Finds the file name.
    //Use filePath.rfind to find the first period, which marks the stopping position for the substring.
    //Use filePath.rfind to find the first slash (/ or \) marking the start position.
    //Returns the file name (case sensitive)

    unsigned extensionPos = filePath.rfind('.');

    if (extensionPos != std::string::npos && extensionPos != filePath.length())
    {
        //If we end up using \ to escape characters in the console this will not work, in this case for windows directories
        unsigned directoryPos = filePath.rfind('\\');
        if (directoryPos != std::string::npos && directoryPos != filePath.length())
        {
            std::string fileName = filePath.substr((directoryPos + 1), (extensionPos + 1));
            return fileName;
        }
        else
        {
            directoryPos = filePath.rfind('/');
            if (directoryPos != std::string::npos && directoryPos != filePath.length())
            {
                std::string fileName = filePath.substr((directoryPos + 1), (extensionPos + 1));
                return fileName;
            }
        }
    }

    return "ERROR";
}

//-----------------------------------------------------------------------------------
bool IncrementPlaycount(const std::string& fileName)
{
    //Increment the playcount for each file type. 
    //Since the default implementation of setProperties (by using a generic TagLib::FileRef)
    //only supports writing a few common tags, we need to use each file type's specific setProperties method.
    //Grab the property map from the file and try to find the PCNT frame. If it doesn't exist, insert it with
    //an initial value. Set the properties on the file and save.

    static const char* PlaycountFrameId = "PCNT";
    std::string fileExtension = GetFileExtension(fileName);

    if (fileExtension == "flac")
    {
        TagLib::FLAC::File flacFile(fileName.c_str());
        TagLib::PropertyMap map = flacFile.properties();
        auto playcountPropertyIter = map.find(PlaycountFrameId);
        if (playcountPropertyIter != map.end())
        {
            bool wasInt = false;
            int currentPlaycount = playcountPropertyIter->second.toString().toInt(&wasInt);
            ASSERT_OR_DIE(&wasInt, "Tried to grab the playcount, but found a non-integer value in the PCNT field.");
            map.replace(PlaycountFrameId, TagLib::String(std::to_string(currentPlaycount + 1)));
        }
        else
        {
            map.insert(PlaycountFrameId, TagLib::String("1"));
        }

        //Create the Xiph comment if it doesn't already exist
        if (!flacFile.hasXiphComment())
        {
            flacFile.xiphComment(1);
        }

        TagLib::Ogg::XiphComment* flacTags = flacFile.xiphComment();
        flacTags->setProperties(map);
        flacFile.save();
    }

    return true;
}

//-----------------------------------------------------------------------------------
Texture* GetImageFromFileMetadata(const std::string& fileName)
{
    //Determine the filetype of fileName
    //If that's an unsupported type (even unsupported for the current point in time), error and return nullptr
    //Else, based on the type, construct the specific object from that filename (ex: TagLib::MPEG::File for mp3 files)
    //Then, grab the picture data (as demonstrated by my hacky code)
    //Pass the image data and size of the image data into CreateUnregisteredTextureFromData
    //Return the result

    unsigned char* srcImage = nullptr;
    unsigned long size;

    //Find the file extension
    std::string fileExtension = GetFileExtension(fileName);
    if (fileExtension == "mp3")
    {
        TagLib::MPEG::File audioFile(fileName.c_str());

        if (audioFile.hasID3v2Tag())
        {
            static const char* IdPicture = "APIC";
            TagLib::ID3v2::Tag* id3v2tag = audioFile.ID3v2Tag();
            TagLib::ID3v2::FrameList Frame;
            TagLib::ID3v2::AttachedPictureFrame* PicFrame;

            //Picture frame
            Frame = id3v2tag->frameListMap()[IdPicture];
            if (!Frame.isEmpty())
            {
                for (TagLib::ID3v2::FrameList::ConstIterator it = Frame.begin(); it != Frame.end(); ++it)
                {
                    PicFrame = (TagLib::ID3v2::AttachedPictureFrame*)(*it);
                    if (PicFrame->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover)
                    {
                        //Extract image (in it�s compressed form)
                        TagLib::ByteVector pictureData = PicFrame->picture();
                        size = pictureData.size();
                        srcImage = (unsigned char*)pictureData.data();
                        if (srcImage)
                        {
                            return Texture::CreateUnregisteredTextureFromData(srcImage, size);
                        }
                    }
                }
            }
        }
    }
    else if (fileExtension == "flac")
    {
        TagLib::FLAC::File audioFile(fileName.c_str());
        auto pictureList = audioFile.pictureList();

        if (!pictureList.isEmpty())
        {
            for (unsigned i = 0; i < audioFile.pictureList().size(); ++i)
            {
                if (pictureList[i]->type() == TagLib::FLAC::Picture::Type::FrontCover)
                {
                    TagLib::ByteVector pictureData = pictureList[i]->data();
                    size = pictureData.size();
                    srcImage = (unsigned char*)pictureData.data();
                    if (srcImage)
                    {
                        return Texture::CreateUnregisteredTextureFromData(srcImage, size);
                    }
                }
            }
        }
        else if (audioFile.hasID3v2Tag())
        {
            static const char* IdPicture = "APIC";
            TagLib::ID3v2::Tag* id3v2tag = audioFile.ID3v2Tag();
            TagLib::ID3v2::FrameList Frame;
            TagLib::ID3v2::AttachedPictureFrame* PicFrame;

            //Picture frame
            Frame = id3v2tag->frameListMap()[IdPicture];
            if (!Frame.isEmpty())
            {
                for (TagLib::ID3v2::FrameList::ConstIterator it = Frame.begin(); it != Frame.end(); ++it)
                {
                    PicFrame = (TagLib::ID3v2::AttachedPictureFrame*)(*it);
                    if (PicFrame->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover)
                    {
                        //Extract image (in it�s compressed form)
                        TagLib::ByteVector pictureData = PicFrame->picture();
                        size = pictureData.size();
                        srcImage = (unsigned char*)pictureData.data();
                        if (srcImage)
                        {
                            return Texture::CreateUnregisteredTextureFromData(srcImage, size);
                        }
                    }
                }
            }
        }
    }
    else if (fileExtension == "wav")
    {
        static const char* IdPicture = "APIC";
        TagLib::RIFF::WAV::File audioFile(fileName.c_str());
        TagLib::ID3v2::Tag* id3v2tag = audioFile.ID3v2Tag();
        TagLib::ID3v2::FrameList Frame;
        TagLib::ID3v2::AttachedPictureFrame* PicFrame;

        if (audioFile.hasID3v2Tag())
        {
            //Picture frame
            Frame = id3v2tag->frameListMap()[IdPicture];
            if (!Frame.isEmpty())
            {
                for (TagLib::ID3v2::FrameList::ConstIterator it = Frame.begin(); it != Frame.end(); ++it)
                {
                    PicFrame = (TagLib::ID3v2::AttachedPictureFrame*)(*it);
                    if (PicFrame->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover)
                    {
                        //Extract image (in it�s compressed form)
                        TagLib::ByteVector pictureData = PicFrame->picture();
                        size = pictureData.size();
                        srcImage = (unsigned char*)pictureData.data();
                        if (srcImage)
                        {
                            return Texture::CreateUnregisteredTextureFromData(srcImage, size);
                        }
                    }
                }
            }
        }
    }

    Console::instance->PrintLine("Could not load album art from song!", RGBA::RED);
    return nullptr;
}

