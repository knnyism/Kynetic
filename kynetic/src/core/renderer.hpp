//
// Created by kennypc on 11/5/25.
//

#pragma once

namespace kynetic
{
class Renderer
{
    friend class Engine;

    AllocatedImage m_draw_image;

    DeletionQueue m_deletion_queue;

    int m_frame_count{0};

public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    void draw_background() const;
    void render();
};
}  // namespace kynetic