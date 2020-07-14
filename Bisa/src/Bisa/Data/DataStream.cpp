#include "bapch.h"

#include "DataStream.h"

namespace Bisa {
    
    DataStream::DataStream(const Config& config)
     : config_(config)
    {
        init();
    }

    DataStream::~DataStream()
    {
        shutdown();
    }

    bool DataStream::fill_next_event()
    {
        while (!file_finished_ && hits_buffer_.size() < 20)
        {
            // BA_CORE_INFO("Hits buffer size: {}", hitsBuffer_.size());
            fill_buffer_with_next_packet();
        }
        if (hits_buffer_.size() > 0)
        {
            new_event_callback_(hits_buffer_.front());
            hits_buffer_.pop_front();
            return true;
        }
        return false;
    }

    void DataStream::fill_buffer_with_next_packet()
    {
        // Find the start of the next packet.
        // If no next packet exists, set file finished to true
        char c;
        while (data_file_.get(c))
        {
            if (c == '=') break;
        }
        if (c != '=')
        {
            file_finished_ = true;
            return;
        }
        
        // Read packet into data
        int num_bytes;
        data_file_ >> num_bytes;
        char* data = new char[num_bytes*2 + 1];
        data[num_bytes*2] = '\0';

        // file_stream_.ignore(40); // ignore the header
        for (int i = 0; i < num_bytes; i++) {
            data_file_.ignore(1);
            data_file_.read(data+i*2, 2);
        }
        Packet packet(data);
        
        // Some error checking
        bool ok = true;
        if (data[2*num_bytes - 2] != 'a' || data[2*num_bytes - 1] != 'a')
        {
            BA_CORE_WARN("Skipping packet: Missing trailing 'aa'");
            ok = false;
        }
        // If error word (not start in 4...)
        if (data[2*13] != '4')
        {
            BA_CORE_WARN("Skipping packet: Error word (byte does not start in 4)");
            ok = false;
        }
        // If number of bytes is < 24, this is an error. Log it
        if (num_bytes < 24) 
        {
            BA_CORE_WARN("Skipping packet: {} bytes", num_bytes);
            ok = false;
        }
        // If number of bytes is > 10000, this is an error. Log it
        if (num_bytes > 1000) 
        {
            BA_CORE_WARN("Skipping packet: {} bytes", num_bytes);
            ok = false;
        }

        // If no errors, continue
        // Otherwise, move onto next packet
        if (!ok) return fill_buffer_with_next_packet();

        HitCollection hits = decode_packet(packet);
        auto it = hits_buffer_.begin();
        for (; it != hits_buffer_.end(); it++)
        {
            if (hits.trigger_id() == it->trigger_id())
            {
                // BA_CORE_TRACE("Merging hits with Trigger ID = {} in buffer", hits.triggerId());
                it->add(hits);
                break;
            }
        }
        if (it == hits_buffer_.end())
        {
            // BA_CORE_TRACE("Adding packet with Trigger ID = {} to buffer", hits.triggerId());
            hits_buffer_.emplace_back(hits);
        }
    }

    void DataStream::init()
    {   
        pairmode_ = config_.pairmode();

        BA_CORE_INFO("Creating data stream from file: {0}", config_.path_to_data());

        data_file_.open(config_.path_to_data());
        BA_CORE_ASSERT(data_file_.is_open(), "Could not open file!")

        id_counter_ = IdCounter(255);
    }

    void DataStream::shutdown()
    {
    }

    HitCollection DataStream::decode_packet(Packet packet)
    {
        HitCollection hits;
        for (auto word_index = 0; word_index < packet.num_words(); word_index++)
        {
            Hit new_hit;
            new_hit.set_trigger_id(id_counter_.get(Packet::slice(packet.fpga_header(), 8, 16)));
            new_hit.set_bcid_fpga(Packet::slice(packet.fpga_header(), 0, 8));
            new_hit.set_felix_counter((packet.raw_bits(26+(packet.num_words()+1)*8, 2)));

            // Handle TDCs 16+17 which cannot be encoded in 4 bits
            int elink = Packet::slice(packet.raw_bits(8, 2), 2, 4);
            if (elink == 8)
            {
                new_hit.set_tdc(16 + Packet::slice(packet.word(word_index), 24, 4));
            }
            else
            {
                int tdc = Packet::slice(packet.word(word_index), 24, 4);
                // int channel = Packet::slice(packet.word(word_index), 19, 5);
                // if (tdc == 4 && config_.strip(channel) < 16)
                // {
                //     tdc = 5;
                // }
                // else if (tdc == 5 && config_.strip(channel) < 16)
                // {
                //     tdc = 4;
                // }
                // else if (tdc == 7 && config_.strip(channel) > 23)
                // {
                //     tdc = 8;
                // }
                // else if (tdc == 8 && config_.strip(channel) > 23)
                // {
                //     tdc = 7;
                // }
                new_hit.set_tdc(tdc);
            }
            
            new_hit.set_channel(Packet::slice(packet.word(word_index), 19, 5));
            new_hit.set_width(pairmode_ ? Packet::slice(packet.word(word_index), 12, 7) : 0);
            new_hit.set_bcid_tdc(pairmode_ ? Packet::slice(packet.word(word_index), 7, 5) : Packet::slice(packet.word(word_index), 7, 12));
            new_hit.set_fine_time(Packet::slice(packet.word(word_index), 0, 7));

            // BA_CORE_TRACE("Added new hit to hits ({})", newHit.toString());
            hits.add(CreateRef<Hit>(new_hit));
        }
        // BA_CORE_TRACE("Finished decoding packet ({})", hits.toString());
        return hits;
    }

}