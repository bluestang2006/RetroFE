/* This file is part of RetroFE.
 *
 * RetroFE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RetroFE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RetroFE.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "VLCVideo.h"
#include "../Graphics/ViewInfo.h"
#include "../Graphics/Component/Image.h"
#include "../Database/Configuration.h"
#include "../Utility/Log.h"
#include "../Utility/Utils.h"
#include "../SDL.h"
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <SDL2/SDL.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vlc/vlc.h>

bool VLCVideo::initialized_ = false;

libvlc_instance_t* VLCVideo::VLC = NULL;

VLCVideo::VLCVideo( int monitor )
    : texture_(NULL)
    , height_(0)
    , width_(0)
    , frameReady_(false)
    , isPlaying_(false)
    , playCount_(0)
    , numLoops_(0)
    , volume_(0.0)
    , currentVolume_(0.0)
    , monitor_(monitor)
{
    paused_ = false;
}
VLCVideo::~VLCVideo()
{
    stop();
}

void VLCVideo::setNumLoops(int n)
{
    if ( n > 0 )
        numLoops_ = n;
}

SDL_Texture *VLCVideo::getTexture() const
{
    return texture_;
}

//void VLCVideo::processNewBuffer (GstElement * /* fakesink /, GstBuffer *buf, GstPad *new_pad, gpointer userdata)
/*{
    VLCVideo *video = (VLCVideo *)userdata;

    SDL_LockMutex(SDL::getMutex());
    if (!video->frameReady_ && video && video->isPlaying_)
    {
        if(!video->width_ || !video->height_)
        {
            GstCaps *caps = gst_pad_get_current_caps (new_pad);
            GstStructure *s = gst_caps_get_structure(caps, 0);

            gst_structure_get_int(s, "width", &video->width_);
            gst_structure_get_int(s, "height", &video->height_);
        }

        if(video->height_ && video->width_ && !video->videoBuffer_)
        {
            video->videoBuffer_ = gst_buffer_ref(buf);
            video->frameReady_ = true;
        }
    }
    SDL_UnlockMutex(SDL::getMutex());
}*/

bool VLCVideo::initialize()
{
    if(initialized_)
    {
        return true;
    }

    const char** args;
    const char* singleargs[] = { "--quiet" };
    int argslen = 0;

    argslen = sizeof(singleargs) / sizeof(singleargs[0]);
    args = singleargs;
    VLC = libvlc_new(argslen, args);

#ifdef WIN32
    //GstRegistry *registry = gst_registry_get();
    //gst_registry_scan_path(registry, path.c_str());
#endif

    initialized_ = true;
    paused_      = false;

    return true;
}

bool VLCVideo::deInitialize()
{
    if(MediaPlayer)
    {
        libvlc_media_player_release(MediaPlayer);
        MediaPlayer = NULL;
    }

    if(Media)
    {
        libvlc_media_release(Media);
        Media = NULL;
    }

    initialized_ = false;
    paused_      = false;
    return true;
}


bool VLCVideo::stop()
{

    paused_ = false;

    if(!initialized_)
    {
        return false;
    }

    if(MediaPlayer)
    {
        libvlc_media_player_release(MediaPlayer);
        MediaPlayer = NULL;
    }

    if(texture_)
    {
        SDL_DestroyTexture(texture_);
        texture_ = NULL;
    }

    isPlaying_ = false;
    height_ = 0;
    width_ = 0;
    frameReady_ = false;

    return true;
}

bool VLCVideo::play(std::string file)
{

    playCount_ = 0;

    if(!initialized_)
    {
        return false;
    }

    currentFile_ = file;

    std::string path = Utils::combinePath(Configuration::absolutePath, "Core");
    printf("DEBUG - %s\n", file.c_str());
    Media = libvlc_media_new_path(VLC, file.c_str());

    if(Media)
    {
        //unsigned track_count;
        libvlc_media_parse_with_options(Media, libvlc_media_parse_local , -1);
        //libvlc_media_parse(Media);
        //libvlc_media_track_t** tracks;
        //track_count = libvlc_media_tracks_get(Media, &tracks);
        //libvlc_media_tracks_release(tracks, track_count);
        MediaPlayer = libvlc_media_player_new_from_media(Media);
        if(libvlc_media_player_play(MediaPlayer) == -1)
        {
            Logger::write( Logger::ZONE_INFO, "VLC", "Error - libvlc_media_player_play(MediaPlayer)" );
            return false;
        }
        else
            Logger::write( Logger::ZONE_INFO, "VLC", "Success - libvlc_media_player_play(MediaPlayer)" );
        libvlc_video_set_callbacks(MediaPlayer, lock, unlock, display, (void*)&Context);
        libvlc_video_set_format(MediaPlayer, "RGBA", VLC::getWidth(), VLC::getHeight(), VLC::getWidth() * 4);
    }
    return true;
}

/*void VLCVideo::freeElements()
{
    if(videoBus_)
    {
        gst_object_unref(videoBus_);
        videoBus_ = NULL;
    }
    if(playbin_)
    {
        gst_object_unref(playbin_);
        playbin_ = NULL;
    }
    if(videoConvertCaps_)
    {
        gst_caps_unref(videoConvertCaps_);
        videoConvertCaps_ = NULL;
    }
    videoSink_    = NULL;
    videoConvert_ = NULL;
    videoBin_     = NULL;
}*/


int VLCVideo::getHeight()
{
    return static_cast<int>(height_);
}

int VLCVideo::getWidth()
{
    return static_cast<int>(width_);
}


void VLCVideo::draw()
{
    frameReady_ = false;
}

void VLCVideo::update(float /* dt */)
{
    SDL_LockMutex(SDL::getMutex());
    if(!texture_ && width_ != 0 && height_ != 0)
    {
        texture_ = SDL_CreateTexture(SDL::getRenderer(monitor_), SDL_PIXELFORMAT_IYUV,
                                    SDL_TEXTUREACCESS_STREAMING, width_, height_);
        SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_BLEND);
    }

	/*if(playbin_)
	{
		if(volume_ > 1.0)
			volume_ = 1.0;
        if ( currentVolume_ > volume_ || currentVolume_ + 0.005 >= volume_ )
            currentVolume_ = volume_;
        else
            currentVolume_ += 0.005;
		gst_stream_volume_set_volume( GST_STREAM_VOLUME( playbin_ ), GST_STREAM_VOLUME_FORMAT_LINEAR, static_cast<double>(currentVolume_));
		if(currentVolume_ < 0.1)
			gst_stream_volume_set_mute( GST_STREAM_VOLUME( playbin_ ), true );
		else
			gst_stream_volume_set_mute( GST_STREAM_VOLUME( playbin_ ), false );
	}

    if(videoBuffer_)
    {
        GstVideoMeta *meta;
        meta = gst_buffer_get_video_meta(videoBuffer_);

        // Presence of meta indicates non-contiguous data in the buffer
        if (!meta)
        {
            void *pixels;
            int pitch;
            unsigned int vbytes = width_ * height_;
            vbytes += (vbytes / 2);
            gsize bufSize = gst_buffer_get_size(videoBuffer_);

            if (bufSize == vbytes)
            {
                SDL_LockTexture(texture_, NULL, &pixels, &pitch);
                gst_buffer_extract(videoBuffer_, 0, pixels, vbytes);
                SDL_UnlockTexture(texture_);
            }
            else
            {
                GstMapInfo bufInfo;
                unsigned int y_stride, u_stride, v_stride;
                const Uint8 *y_plane, *u_plane, *v_plane;

                y_stride = GST_ROUND_UP_4(width_);
                u_stride = v_stride = GST_ROUND_UP_4(y_stride / 2);

                gst_buffer_map(videoBuffer_, &bufInfo, GST_MAP_READ);
                y_plane = bufInfo.data;
                u_plane = y_plane + (height_ * y_stride);
                v_plane = u_plane + ((height_ / 2) * u_stride);
                SDL_UpdateYUVTexture(texture_, NULL,
                                     (const Uint8*)y_plane, y_stride,
                                     (const Uint8*)u_plane, u_stride,
                                     (const Uint8*)v_plane, v_stride);
                gst_buffer_unmap(videoBuffer_, &bufInfo);
            }
        }
        else
        {
            GstMapInfo y_info, u_info, v_info;
            void *y_plane, *u_plane, *v_plane;
            int y_stride, u_stride, v_stride;

            gst_video_meta_map(meta, 0, &y_info, &y_plane, &y_stride, GST_MAP_READ);
            gst_video_meta_map(meta, 1, &u_info, &u_plane, &u_stride, GST_MAP_READ);
            gst_video_meta_map(meta, 2, &v_info, &v_plane, &v_stride, GST_MAP_READ);
            SDL_UpdateYUVTexture(texture_, NULL,
                                 (const Uint8*)y_plane, y_stride,
                                 (const Uint8*)u_plane, u_stride,
                                 (const Uint8*)v_plane, v_stride);
            gst_video_meta_unmap(meta, 0, &y_info);
            gst_video_meta_unmap(meta, 1, &u_info);
            gst_video_meta_unmap(meta, 2, &v_info);
        }

        gst_buffer_unref(videoBuffer_);
        videoBuffer_ = NULL;
    }

    if(videoBus_)
    {
        GstMessage *msg = gst_bus_pop(videoBus_);
        if(msg)
        {
            if(GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS)
            {
                playCount_++;

                //todo: nesting hazard
                // if number of loops is 0, set to infinite (todo: this is misleading, rename variable)
                if(!numLoops_ || numLoops_ > playCount_)
                {
                    gst_element_seek(playbin_,
                                     1.0,
                                     GST_FORMAT_TIME,
                                     GST_SEEK_FLAG_FLUSH,
                                     GST_SEEK_TYPE_SET,
                                     0,
                                     GST_SEEK_TYPE_NONE,
                                     GST_CLOCK_TIME_NONE);
                }
                else
                {
                    isPlaying_ = false;
                }
            }

            gst_message_unref(msg);
        }
    }*/
    SDL_UnlockMutex(SDL::getMutex());
}


bool VLCVideo::isPlaying()
{
    return isPlaying_;
}


void VLCVideo::setVolume(float volume)
{
    volume_ = volume;
}


void VLCVideo::skipForward( )
{

    if ( !isPlaying_ )
        return;

    //

}

void VLCVideo::skipBackward( )
{

    if ( !isPlaying_ )
        return;

    //

}


void VLCVideo::skipForwardp( )
{

    if ( !isPlaying_ )
        return;

    //

}

void VLCVideo::skipBackwardp( )
{

    if ( !isPlaying_ )
        return;

    //

}


void VLCVideo::pause( )
{
    paused_ = !paused_;
    return;

}


void VLCVideo::restart( )
{

    if ( !isPlaying_ )
        return;

    //gst_element_seek_simple( playbin_, GST_FORMAT_TIME, GstSeekFlags( GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT ), 0 );

}


unsigned long long VLCVideo::getCurrent( )
{
    int ret = 0;
    return (unsigned long long)ret;
}


unsigned long long VLCVideo::getDuration( )
{
    int ret = 0;
    return (unsigned long long)ret;
}


bool VLCVideo::isPaused( )
{
    return paused_;
}
