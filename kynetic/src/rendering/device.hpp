//
// Created by kennypc on 11/4/25.
//

#pragma once

namespace kynetic
{
class Device
{
public:
    Device();
    ~Device();

    Device(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(const Device&) = delete;
    Device& operator=(Device&&) = delete;

    void begin_frame() const;
    void end_frame() const;
};
}  // namespace kynetic