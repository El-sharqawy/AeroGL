#include "Matrix4.h"
#include "Matrix3.h"

Matrix3 Matrix3_InitMatrix4(const Matrix4 mat4)
{
    Matrix3 result;
    // Upper 3x3 columns
    result.cols[0] = (Vector3){ mat4.cols[0].x, mat4.cols[1].x, mat4.cols[2].x, 0.0f };
    result.cols[1] = (Vector3){ mat4.cols[0].y, mat4.cols[1].y, mat4.cols[2].y, 0.0f };
    result.cols[2] = (Vector3){ mat4.cols[0].z, mat4.cols[1].z, mat4.cols[2].z, 0.0f };
    return result;
}
