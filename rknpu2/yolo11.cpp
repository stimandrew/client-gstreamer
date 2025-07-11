
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../yolo11.h"
#include "common.h"
#include "file_utils.h"
#include "image_utils.h"
#include <QDebug>

YOLO11Processor::YOLO11Processor(QObject *parent) : QObject(parent) {
    memset(&rknn_app_ctx, 0, sizeof(rknn_app_context_t));
    init_post_process();
}

YOLO11Processor::~YOLO11Processor() {
    QMutexLocker locker(&m_mutex);
    if (m_initialized) {
        release_yolo11_model(&rknn_app_ctx);
    }
    deinit_post_process();
}

bool YOLO11Processor::isInitialized() const {
    QMutexLocker locker(&m_mutex);
    return m_initialized;
}

bool YOLO11Processor::initialize(const char* model_path) {
     QMutexLocker locker(&m_mutex);

    if (m_initialized) {
        release_yolo11_model(&rknn_app_ctx);
    }

    int ret = init_yolo11_model(model_path, &rknn_app_ctx);
    if (ret != 0) {
        qWarning() << "init_yolo11_model fail! ret=" << ret << "model_path=" << model_path;
        return false;
    }

    m_initialized = true;
    return true;
}

void YOLO11Processor::processFrame(const QImage& frame) {
    QMutexLocker locker(&m_mutex);

    if (m_processed) {
        return;
    }

    if (!m_initialized) {
        qWarning() << "YOLO processor not initialized";
        emit objectsDetected(QList<QRect>(), frame);
        return;
    }

    if (frame.isNull()) {
        qWarning() << "Received null frame";
        emit objectsDetected(QList<QRect>(), frame);
        return;
    }

    // Проверяем размер изображения
    if (frame.width() < 64 || frame.height() < 64) {
        qWarning() << "Frame size too small:" << frame.size();
        emit objectsDetected(QList<QRect>(), frame);
        return;
    }

    QImage rgbFrame = frame.convertToFormat(QImage::Format_RGB888);
    if (rgbFrame.isNull()) {
        qWarning() << "Failed to convert frame to RGB888";
        emit objectsDetected(QList<QRect>(), frame);
        return;
    }

    object_detect_result_list od_results;
    memset(&od_results, 0, sizeof(od_results));

    if (processImage(rgbFrame, &od_results)) {
        QList<QRect> objects;
        for (int i = 0; i < od_results.count; i++) {
            auto& res = od_results.results[i];
            objects.append(QRect(res.box.left, res.box.top,
                                 res.box.right - res.box.left,
                                 res.box.bottom - res.box.top));
        }
        emit objectsDetected(objects, frame);
    } else {
        qWarning() << "Failed to process image with YOLO";
        emit objectsDetected(QList<QRect>(), frame);
    }

     m_processed = true;
}

bool YOLO11Processor::processImage(const QImage& image, object_detect_result_list* od_results) {
    image_buffer_t src_image;
    if (!convertQImageToImageBuffer(image, &src_image)) {
        qWarning() << "Failed to convert QImage to image buffer";
        return false;
    }

    int ret = inference_yolo11_model(&rknn_app_ctx, &src_image, od_results);
    if (ret != 0) {
        qWarning() << "Failed to process image with YOLO, error code:" << ret;
        return false;
    }

    return true;
}

void YOLO11Processor::release() {
    QMutexLocker locker(&m_mutex);

    if (m_initialized) {
        release_yolo11_model(&rknn_app_ctx);
        m_initialized = false;
    }
}

bool YOLO11Processor::convertQImageToImageBuffer(const QImage& qimage, image_buffer_t* src_image) {
    if (qimage.format() != QImage::Format_RGB888 ||
        qimage.width() != 640 ||
        qimage.height() != 640) {
        return false;
    }

    // Выделяем память для буфера
    src_image->width = 640;
    src_image->height = 640;
    src_image->format = IMAGE_FORMAT_RGB888;
    src_image->size = 640 * 640 * 3;
    src_image->virt_addr = (unsigned char*)malloc(src_image->size);

    if (!src_image->virt_addr) {
        return false;
    }

    // Копируем данные с учетом возможного выравнивания строк в QImage
    for (int y = 0; y < 640; y++) {
        const uchar* scanLine = qimage.scanLine(y);
        memcpy((uchar*)src_image->virt_addr + y * 640 * 3, scanLine, 640 * 3);
    }

    return true;
}

static void dump_tensor_attr(rknn_tensor_attr *attr)
{
    printf("  index=%d, name=%s, n_dims=%d, dims=[%d, %d, %d, %d], n_elems=%d, size=%d, fmt=%s, type=%s, qnt_type=%s, "
           "zp=%d, scale=%f\n",
           attr->index, attr->name, attr->n_dims, attr->dims[0], attr->dims[1], attr->dims[2], attr->dims[3],
           attr->n_elems, attr->size, get_format_string(attr->fmt), get_type_string(attr->type),
           get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
}

int init_yolo11_model(const char *model_path, rknn_app_context_t *app_ctx)
{
    int ret;
    int model_len = 0;
    char *model;
    rknn_context ctx = 0;

    // Load RKNN Model
    model_len = read_data_from_file(model_path, &model);
    if (model == NULL)
    {
        printf("load_model fail!\n");
        return -1;
    }

    ret = rknn_init(&ctx, model, model_len, 0, NULL);
    free(model);
    if (ret < 0)
    {
        printf("rknn_init fail! ret=%d\n", ret);
        return -1;
    }

    // Get Model Input Output Number
    rknn_input_output_num io_num;
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret != RKNN_SUCC)
    {
        printf("rknn_query fail! ret=%d\n", ret);
        return -1;
    }
    printf("model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);

    // Get Model Input Info
    printf("input tensors:\n");
    rknn_tensor_attr input_attrs[io_num.n_input];
    memset(input_attrs, 0, sizeof(input_attrs));
    for (int i = 0; i < io_num.n_input; i++)
    {
        input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC)
        {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(input_attrs[i]));
    }

    // Get Model Output Info
    printf("output tensors:\n");
    rknn_tensor_attr output_attrs[io_num.n_output];
    memset(output_attrs, 0, sizeof(output_attrs));
    for (int i = 0; i < io_num.n_output; i++)
    {
        output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC)
        {
            printf("rknn_query fail! ret=%d\n", ret);
            return -1;
        }
        dump_tensor_attr(&(output_attrs[i]));
    }

    // Set to context
    app_ctx->rknn_ctx = ctx;

    // TODO
    if (output_attrs[0].qnt_type == RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC && output_attrs[0].type == RKNN_TENSOR_INT8)
    {
        app_ctx->is_quant = true;
    }
    else
    {
        app_ctx->is_quant = false;
    }

    app_ctx->io_num = io_num;
    app_ctx->input_attrs = (rknn_tensor_attr *)malloc(io_num.n_input * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->input_attrs, input_attrs, io_num.n_input * sizeof(rknn_tensor_attr));
    app_ctx->output_attrs = (rknn_tensor_attr *)malloc(io_num.n_output * sizeof(rknn_tensor_attr));
    memcpy(app_ctx->output_attrs, output_attrs, io_num.n_output * sizeof(rknn_tensor_attr));

    if (input_attrs[0].fmt == RKNN_TENSOR_NCHW)
    {
        printf("model is NCHW input fmt\n");
        app_ctx->model_channel = input_attrs[0].dims[1];
        app_ctx->model_height = input_attrs[0].dims[2];
        app_ctx->model_width = input_attrs[0].dims[3];
    }
    else
    {
        printf("model is NHWC input fmt\n");
        app_ctx->model_height = input_attrs[0].dims[1];
        app_ctx->model_width = input_attrs[0].dims[2];
        app_ctx->model_channel = input_attrs[0].dims[3];
    }
    printf("model input height=%d, width=%d, channel=%d\n",
           app_ctx->model_height, app_ctx->model_width, app_ctx->model_channel);

    return 0;
}

int release_yolo11_model(rknn_app_context_t *app_ctx)
{
    if (app_ctx->input_attrs != NULL)
    {
        free(app_ctx->input_attrs);
        app_ctx->input_attrs = NULL;
    }
    if (app_ctx->output_attrs != NULL)
    {
        free(app_ctx->output_attrs);
        app_ctx->output_attrs = NULL;
    }
    if (app_ctx->rknn_ctx != 0)
    {
        rknn_destroy(app_ctx->rknn_ctx);
        app_ctx->rknn_ctx = 0;
    }
    return 0;
}

int inference_yolo11_model(rknn_app_context_t *app_ctx, image_buffer_t *img, object_detect_result_list *od_results)
{
    int ret;
    image_buffer_t dst_img;
    letterbox_t letter_box;
    rknn_input inputs[app_ctx->io_num.n_input];
    rknn_output outputs[app_ctx->io_num.n_output];
    const float nms_threshold = NMS_THRESH;      // 默认的NMS阈值
    const float box_conf_threshold = BOX_THRESH; // 默认的置信度阈值
    int bg_color = 114;

    if ((!app_ctx) || !(img) || (!od_results))
    {
        return -1;
    }

    memset(od_results, 0x00, sizeof(*od_results));
    memset(&letter_box, 0, sizeof(letterbox_t));
    memset(&dst_img, 0, sizeof(image_buffer_t));
    memset(inputs, 0, sizeof(inputs));
    memset(outputs, 0, sizeof(outputs));

    // Pre Process
    dst_img.width = app_ctx->model_width;
    dst_img.height = app_ctx->model_height;
    dst_img.format = IMAGE_FORMAT_RGB888;
    dst_img.size = get_image_size(&dst_img);
    dst_img.virt_addr = (unsigned char *)malloc(dst_img.size);
    if (dst_img.virt_addr == NULL)
    {
        printf("malloc buffer size:%d fail!\n", dst_img.size);
        return -1;
    }

    // letterbox
    ret = convert_image_with_letterbox(img, &dst_img, &letter_box, bg_color);
    if (ret < 0)
    {
        printf("convert_image_with_letterbox fail! ret=%d\n", ret);
        return -1;
    }

    // Set Input Data
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_UINT8;
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    inputs[0].size = app_ctx->model_width * app_ctx->model_height * app_ctx->model_channel;
    inputs[0].buf = dst_img.virt_addr;

    ret = rknn_inputs_set(app_ctx->rknn_ctx, app_ctx->io_num.n_input, inputs);
    if (ret < 0)
    {
        printf("rknn_input_set fail! ret=%d\n", ret);
        return -1;
    }

    // Run
    printf("rknn_run\n");
    ret = rknn_run(app_ctx->rknn_ctx, nullptr);
    if (ret < 0)
    {
        printf("rknn_run fail! ret=%d\n", ret);
        return -1;
    }

    // Get Output
    memset(outputs, 0, sizeof(outputs));
    for (int i = 0; i < app_ctx->io_num.n_output; i++)
    {
        outputs[i].index = i;
        outputs[i].want_float = (!app_ctx->is_quant);
    }
    ret = rknn_outputs_get(app_ctx->rknn_ctx, app_ctx->io_num.n_output, outputs, NULL);
    if (ret < 0)
    {
        printf("rknn_outputs_get fail! ret=%d\n", ret);
        goto out;
    }

    // Post Process
    post_process(app_ctx, outputs, &letter_box, box_conf_threshold, nms_threshold, od_results);

    // Remeber to release rknn output
    rknn_outputs_release(app_ctx->rknn_ctx, app_ctx->io_num.n_output, outputs);

out:
    if (dst_img.virt_addr != NULL)
    {
        free(dst_img.virt_addr);
    }

    return ret;
}
