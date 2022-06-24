//==============================================================================
//
//  OvenMediaEngine
//
//  Created by Getroot
//  Copyright (c) 2022 AirenSoft. All rights reserved.
//
//==============================================================================
#pragma once

#include <base/common_types.h>
#include <base/info/media_track.h>

class RtcRendition
{
public:
    RtcRendition(const ov::String &name, const std::shared_ptr<const MediaTrack> &video_track, const std::shared_ptr<const MediaTrack> &audio_track)
    {
        _name = name;
        _video_track = video_track;
        _audio_track = audio_track;
    }

    const ov::String &GetName() const
    {
        return _name;
    }
    const std::shared_ptr<const MediaTrack> &GetVideoTrack() const
    {
        return _video_track;
    }
    const std::shared_ptr<const MediaTrack> &GetAudioTrack() const
    {
        return _audio_track;
    }

    Json::Value ToJson() const
    {
        Json::Value json;
        
        json["name"] = _name.CStr();
        json["video_track_name"] = _video_track->GetName().CStr();
        json["audio_track_name"] = _audio_track->GetName().CStr();

        // TODO(ABR) : Add other attributes

        return json;
    }

private:
    ov::String _name;
    std::shared_ptr<const MediaTrack> _video_track;
    std::shared_ptr<const MediaTrack> _audio_track;
};

// Rendition list with same video/audio tracks
class RtcPlaylist
{
public:
    RtcPlaylist(const ov::String &name, cmn::MediaCodecId video_codec_id, cmn::MediaCodecId audio_codec_id)
    {
        _name = name;
        _video_codec_id = video_codec_id;
        _audio_codec_id = audio_codec_id;
    }

    bool AddRendition(const std::shared_ptr<const RtcRendition> &rendition)
    {
        if (_first_rendition == nullptr)
        {
            _first_rendition = rendition;
        }

        _rendition_map.emplace(rendition->GetName(), rendition);

        return true;
    }
    std::shared_ptr<const RtcRendition> GetRendition(const ov::String &name) const
    {
        auto it = _rendition_map.find(name);
        if (it == _rendition_map.end())
        {
            return nullptr;
        }

        return it->second;
    }

    std::shared_ptr<const RtcRendition> GetFirstRendition() const
    {
        return _first_rendition;
    }

    Json::Value ToJson(cmn::MediaCodecId video_codec_id, cmn::MediaCodecId audio_codec_id) const
    {
        Json::Value playlist_json;

        playlist_json["name"] = _name.CStr();

        Json::Value renditions_json;
        for (const auto &it : _rendition_map)
        {
            renditions_json.append(it.second->ToJson());
        }

        playlist_json["renditions"] = renditions_json;

        // TODO(ABR) : Add other attributes

        return playlist_json;
    }

private:
    ov::String _name;
    cmn::MediaCodecId _video_codec_id;
    cmn::MediaCodecId _audio_codec_id;

    std::shared_ptr<const RtcRendition> _first_rendition;
    // Rendition Name : Rendition
    std::map<ov::String, std::shared_ptr<const RtcRendition>> _rendition_map;
};

class RtcMasterPlaylist
{
public:
    RtcMasterPlaylist(const ov::String &name, const ov::String &file_name)
    {
        _name = name;
        _file_name = file_name;
    }

    bool AddRendition(const std::shared_ptr<const RtcRendition> &rendition)
    {
        auto video_codec_id = rendition->GetVideoTrack() ? rendition->GetVideoTrack()->GetCodecId() : cmn::MediaCodecId::None;
        auto audio_codec_id = rendition->GetAudioTrack() ? rendition->GetAudioTrack()->GetCodecId() : cmn::MediaCodecId::None;

        auto playlist = GetPlaylist(video_codec_id, audio_codec_id);
        if (playlist == nullptr)
        {
            playlist = CreatePlaylist(video_codec_id, audio_codec_id);
            _playlist_map.emplace(GetPlaylistKey(video_codec_id, audio_codec_id), playlist);
        }

        playlist->AddRendition(rendition);

        AddPayloadTrack(rendition->GetVideoTrack());
        AddPayloadTrack(rendition->GetAudioTrack());

        return true;
    }

    // Track List for WebRTC Payload
    std::map<cmn::MediaCodecId, std::shared_ptr<const MediaTrack>> GetPayloadTrackMap() const
    {
        return _payload_track_map;
    }

    // Get Playlist
    std::shared_ptr<const RtcPlaylist> GetPlaylist(cmn::MediaCodecId video_codec_id, cmn::MediaCodecId audio_codec_id) const
    {
        auto it = _playlist_map.find(GetPlaylistKey(video_codec_id, audio_codec_id));
        if (it == _playlist_map.end())
        {
            return nullptr;
        }

        auto playlist = it->second;

        return playlist;
    }

    std::shared_ptr<RtcPlaylist> GetPlaylist(cmn::MediaCodecId video_codec_id, cmn::MediaCodecId audio_codec_id)
    {
        auto it = _playlist_map.find(GetPlaylistKey(video_codec_id, audio_codec_id));
        if (it == _playlist_map.end())
        {
            return nullptr;
        }

        return it->second;
    }

private:

    std::shared_ptr<RtcPlaylist> CreatePlaylist(cmn::MediaCodecId video_codec_id, cmn::MediaCodecId audio_codec_id) const
    {
        return std::make_shared<RtcPlaylist>(_name, video_codec_id, audio_codec_id);
    }

    ov::String GetPlaylistKey(cmn::MediaCodecId video_codec_id, cmn::MediaCodecId audio_codec_id) const
    {
        return ov::String::FormatString("%d_%d", video_codec_id, audio_codec_id);
    }

    void AddPayloadTrack(const std::shared_ptr<const MediaTrack> &track)
    {
        if (track == nullptr)
        {
            return;
        }
        
        _payload_track_map.emplace(track->GetCodecId(), track);
    }

    ov::String _name;
    ov::String _file_name;

    // Track list for payload list
    // OME specifies only one payload per codec in WebRTC SDP.
    std::map<cmn::MediaCodecId, std::shared_ptr<const MediaTrack>> _payload_track_map;

    // Key : string(%d_%d), video_codec_id, audio_codec_id
    // Value : Rendition list
    std::map<ov::String, std::shared_ptr<RtcPlaylist>> _playlist_map;
};
