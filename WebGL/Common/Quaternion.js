// Quaternion.js, from the original written in C by Ken Shoemake

function QuaternionFromMatrix(m) {
    // [this routine from Ken's 2015 Arcball demo]
    // Convert rotation matrix (4×4) to unit quaternion.
    var Rxx = m[0][0], Rxy = m[1][0], Rxz = m[2][0],
        Ryx = m[0][1], Ryy = m[1][1], Ryz = m[2][1],
        Rzx = m[0][2], Rzy = m[1][2], Rzz = m[2][2];
    var tr = Rxx + Ryy + Rzz;
    var i, Rii;
    var r, s, x, y, z, w;
    if (tr >= 0.0) {
        // Quat w has max magnitude
        r = Math.sqrt(tr + 1.0);
        s = 0.5 / r;
        x = (Rzy - Ryz) * s;
        y = (Rxz - Rzx) * s;
        z = (Ryx - Rxy) * s;
        w = 0.5 * r;
    } else {
        // Quat w has small magnitude
        // Max diagonal element implies max of x, y, z
        // r = Math.sqrt(Rii - Rjj - Rkk + 1.0);
        // s = 0.5 / r;
        // qi = 0.5 * r;
        // qj = (Rij + Rji) * s;
        // qk = (Rki + Rik) * s;
        // w = (Rkj - Rjk) * s;
        i = 0;
        Rii = Rxx;
        if (Ryy > Rxx) {i = 1; Rii = Ryy;}
        if (Rzz > Rii) {i = 2;}
        switch (i) {
        case 0:
            r = Math.sqrt(Rxx - Ryy - Rzz + 1.0);
            s = 0.5 / r;
            x = 0.5 * r;
            y = (Rxy + Ryx) * s;
            z = (Rzx + Rxz) * s;
            w = (Rzy - Ryz) * s;
            break;
        case 1:
            r = Math.sqrt(Ryy - Rzz - Rxx + 1.0);
            s = 0.5 / r;
            y = 0.5 * r;
            z = (Ryz + Rzy) * s;
            x = (Rxy + Ryx) * s;
            w = (Rxz - Rzx) * s;
            break;
        case 2:
            r = Math.sqrt(Rzz - Rxx - Ryy + 1.0);
            s = 0.5 / r;
            z = 0.5 * r;
            x = (Rzx + Rxz) * s;
            y = (Ryz + Rzy) * s;
            w = (Ryx - Rxy) * s;
            break;
        }
    }
    return vec4(x, y, z, w);
}

function MatrixFromQuaternion(q) {
    var result = mat4();
    var norm = QuaternionNorm(q);
    if (Math.abs(norm) < Number.EPSILON)
        return result;
    var s = 2/norm;
    var xs = q[0]*s,  ys = q[1]*s,  zs = q[2]*s;
    var wx = q[3]*xs, wy = q[3]*ys, wz = q[3]*zs;
    var xx = q[0]*xs, xy = q[0]*ys, xz = q[0]*zs;
    var yy = q[1]*ys, yz = q[1]*zs, zz = q[2]*zs;
    result[0] = vec4(1 - (yy + zz), xy + wz,       xz - wy,       0);
    result[1] = vec4(xy - wz,       1 - (xx + zz), yz + wx,       0);
    result[2] = vec4(xz + wy,       yz - wx,       1 - (xx + yy), 0);
    return result;
}

function QuaternionNorm(q) { return q[0]*q[0]+q[1]*q[1]+q[2]*q[2]+q[3]*q[3]; }

function QuaternionMultiply(q1, q2) {
    var x =  q1[0] * q2[3] + q1[1] * q2[2] - q1[2] * q2[1] + q1[3] * q2[0];
    var y = -q1[0] * q2[2] + q1[1] * q2[3] + q1[2] * q2[0] + q1[3] * q2[1];
    var z =  q1[0] * q2[1] - q1[1] * q2[0] + q1[2] * q2[3] + q1[3] * q2[2];
    var w = -q1[0] * q2[0] - q1[1] * q2[1] - q1[2] * q2[2] + q1[3] * q2[3];
	return vec4(x, y, z, w);
}
