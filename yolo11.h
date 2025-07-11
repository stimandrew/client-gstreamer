#pragma once

#include "include/rknn_api.h"
#include "utils/common.h"
#include <QImage>
#include <QRect>
#include <QMutex>
#include <QObject>



typedef struct {
    rknn_context rknn_ctx;
    rknn_input_output_num io_num;
    rknn_tensor_attr* input_attrs;
    rknn_tensor_attr* output_attrs;

    int model_channel;
    int model_width;
    int model_height;
    bool is_quant;
} rknn_app_context_t;

#include "postprocess.h"


int init_yolo11_model(const char* model_path, rknn_app_context_t* app_ctx);

int release_yolo11_model(rknn_app_context_t* app_ctx);

int inference_yolo11_model(rknn_app_context_t* app_ctx, image_buffer_t* img, object_detect_result_list* od_results);

class YOLO11Processor : public QObject {
    Q_OBJECT
public:
    explicit YOLO11Processor(QObject *parent = nullptr);
    ~YOLO11Processor();

    bool initialize(const char* model_path);
    bool isInitialized() const;
    void release();

public slots:  // Явно объявляем слот для обработки кадров
    void processFrame(const QImage& frame);

signals:
    void objectsDetected(const QList<QRect>& objects, const QImage& frame);

private:
    bool processImage(const QImage& image, object_detect_result_list* od_results);
    bool convertQImageToImageBuffer(const QImage& qimage, image_buffer_t* src_image);

    rknn_app_context_t rknn_app_ctx;
    mutable QMutex m_mutex;
    bool m_initialized = false;
    bool m_processed = false;
};
