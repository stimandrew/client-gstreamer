#include "setplaying.h"

SetPlaying::SetPlaying (GstElement * pipeline)
{
    this->pipeline_ = pipeline ? static_cast<GstElement *> (gst_object_ref (pipeline)) : NULL;
}

SetPlaying::~SetPlaying ()
{
    if (this->pipeline_)
        gst_object_unref (this->pipeline_);
}

void
SetPlaying::run ()
{
    if (this->pipeline_)
        gst_element_set_state (this->pipeline_, GST_STATE_PLAYING);
}
