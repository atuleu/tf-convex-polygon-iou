#define EIGEN_USE_THREADS

#if GOOGLE_CUDA
#define EIGEN_USE_GPU
#endif // GOOGLE_CUDA

#include "tf_convex_quad_iou/custom_ops/iou/cc/kernels/iou_matrix_ops.h"

#include "tensorflow/core/framework/register_types.h"
#include "tensorflow/core/platform/threadpool.h"

namespace tensorflow {
namespace convex_quad_iou {

using CPUDevice = Eigen::ThreadPoolDevice;
using GPUDevice = Eigen::GpuDevice;

namespace functor {

template <typename T>
struct IoUMatrixFunctor<CPUDevice, T> {
	void operator()(OpKernelContext* ctx, const CPUDevice& d,
	                const T* __restrict__ anchors,
	                const T* __restrict__ quads,
	                T* __restrict__ output,
	                const int anchors_size,
	                const int quads_size) {
		auto thread_pool = ctx->device()->tensorflow_cpu_worker_threads()->workers;
		auto toLoop =
			[&anchors,&quads,&output,quads_size](int64 startIndex,int64 endIndex) {
				for ( int i = startIndex; i < endIndex; ++i) {
					int anchorsIndex = i / quads_size;
					int quadsIndex = i - quads_size * anchorsIndex;
					output[i] = ComputeIoU<T>(anchors + 8 * anchorsIndex,
					                          quads + 8 * quadsIndex);
				}
			};

		thread_pool->ParallelFor(anchors_size * quads_size,
		                         10000,
		                         toLoop);
	}
};

} // namespace functor


template <typename Device, typename T>
class IoUMatrixOp : public OpKernel {
public:
	explicit IoUMatrixOp(OpKernelConstruction * ctx) : OpKernel(ctx) {}

	void Compute(OpKernelContext * ctx) override {
		const Tensor & anchors = ctx->input(0);
		const Tensor & quads = ctx->input(1);

		const TensorShape & anchorsShape = anchors.shape();
		const TensorShape & quadsShape = quads.shape();


		OP_REQUIRES(ctx,
		            anchorsShape.dims() == 3
		            && anchorsShape.dim_size(1) == 4
		            && anchorsShape.dim_size(2) == 2,
		            errors::InvalidArgument("anchors must be of size [None, 4, 2], but is: ",
		                                    anchorsShape.DebugString()));
		OP_REQUIRES(ctx,
		            quadsShape.dims() == 3
		            && quadsShape.dim_size(1) == 4
		            && quadsShape.dim_size(2) == 2,
		            errors::InvalidArgument("quads must be of size [None, 4, 2], but is: ",
		                                    quadsShape.DebugString()));

		TensorShape outputShape;
		int anchorsSize = anchorsShape.dim_size(0);
		int quadsSize = quadsShape.dim_size(0);

		OP_REQUIRES_OK(ctx,TensorShapeUtils::MakeShape(gtl::ArraySlice<int32>({anchorsSize,quadsSize}),
		                                               &outputShape));

		Tensor * output = nullptr;
		OP_REQUIRES_OK(ctx,ctx->allocate_output(0,outputShape,&output));

		if (anchorsSize <= 0 || quadsSize <= 0) {
			return;
		}
		functor::IoUMatrixFunctor<Device,T>()(ctx,ctx->eigen_device<Device>(),
		                                      anchors.flat<T>().data(),
		                                      quads.flat<T>().data(),
		                                      output->flat<T>().data(),
		                                      anchorsSize,
		                                      quadsSize);
	}

private:
	TF_DISALLOW_COPY_AND_ASSIGN(IoUMatrixOp);
};

#define REGISTER(TYPE)	  \
	REGISTER_KERNEL_BUILDER(Name("ConvexQuadIoU>IoUMatrix").Device(DEVICE_CPU).TypeConstraint<TYPE>("T"), \
	                        IoUMatrixOp<CPUDevice, TYPE>);

TF_CALL_half(REGISTER);
TF_CALL_float(REGISTER);
TF_CALL_double(REGISTER);

#undef REGISTER

#if GOOGLE_CUDA
#define REGISTER(TYPE)                                                       \
	REGISTER_KERNEL_BUILDER(Name("ConvexQuadIoU>IoUMatrix").Device(DEVICE_GPU).TypeConstraint<TYPE>("T"), \
	                        IoUMatrixOp<GPUDevice, TYPE>)

TF_CALL_half(REGISTER);
TF_CALL_float(REGISTER);
TF_CALL_double(REGISTER);
#undef REGISTER
#endif  // GOOGLE_CUDA

} // namespace convex_quad_iou
} // namespace tensorflow
