/*---------- ArcBall class implementation*/
function ArcBall(){
    this.AdjustWidth = null
    this.AdjustHeight = null;
    this.StVec = null;
    this.EnVec = null;
    this.Transform = null;
    this.ThisRot = null;
    this.LastRot = null;
    this.Quat = null;
    this.position = [0,0,-1];
    this.lookAt = [0,0,0];
}

ArcBall.prototype = {
    setBounds : function(newWidth, newHeight){
        this.AdjustWidth =  1.0 / ((newWidth  - 1.0) * 0.5);
        this.AdjustHeight = 1.0 / ((newHeight - 1.0) * 0.5);

        this.init();
    },
    
    click : function(X,Y){
        this.StVec = this.mapToSphere(X,Y);
    },

    mapToSphere : function(X,Y){
        var P = {x:0,y:0,z:0}; 

        //Adjust point coords and scale down to range of [-1 ... 1]
        P.x = (X * this.AdjustWidth) - 1.0;
        P.y = 1.0 - (Y * this.AdjustHeight);

        //Compute the square of the length of the vector to the point from the center
        var length = P.x*P.x  + P.y*P.y;

        //If the point is mapped outside of the sphere... (length > radius squared)
        if(length > 1.0){
            //Compute a normalizing factor (radius / sqrt(length))
            var norm = 1.0/Math.sqrt(length);

            //Return the "normalized" vector, a point on the sphere
            P.x *= norm;
            P.y *= norm;
            P.z = 0.0;
            return [P.x,P.y,P.z];
        } else {
            //Return a vector to a point mapped inside the sphere sqrt(radius squared - length)
            return [P.x,P.y,Math.sqrt(1.0-length)];
        }
    },

    drag : function(X,Y){
        this.EnVec = this.mapToSphere(X,Y);

        //Compute the vector perpendicular to the begin and end vectors
        var Perp = cross(this.StVec,this.EnVec);

        //Compute the length of the perpendicular vector
        if(Vector3fLength(Perp) > 1.0e-5){//if its non-zero
            //In the quaternion values, w is cosine (theta / 2), where theta is rotation angle
            return [Perp[0],Perp[1],Perp[2],dot(this.StVec,this.EnVec)];
        } else {
            return [0.0,0.0,0.0,0.0]
        }
    },

    init: function(){
        this.Transform = $M([  [1.0,  0.0,  0.0,  0.0],
                           [0.0,  1.0,  0.0,  0.0],
                           [0.0,  0.0,  1.0,  0.0],
                           [0.0,  0.0,  0.0,  1.0] ]);
         
        this.LastRot   = [  1.0,  0.0,  0.0,                  // Last Rotation
                           0.0,  1.0,  0.0,
                           0.0,  0.0,  1.0 ];
         
        this.ThisRot  = [  1.0,  0.0,  0.0,                  // This Rotation
                           0.0,  1.0,  0.0,
                           0.0,  0.0,  1.0 ];

        this.zoomScale = 1.0;
    },

    move: function(X,Y){
       //create a quaternion which captures the rotation of the arcball
        this.Quat = this.drag(X,Y);

       //create a 3x3 matrix of the rotation from the quaternion
        this.ThisRot = Matrix3fSetRotationFromQuat4f(this.Quat);

        //accumulate the current rotation to all previous rotations
        var tmp = ArrayToSylvesterMatrix(this.ThisRot,3)
                    .x(ArrayToSylvesterMatrix(this.LastRot,3))

        //save rotation for next mouse event
        this.ThisRot = SylvesterToArray(tmp);

       //set the final transform matrix that we will multiply by the modelView
       this.Transform = ArrayToSylvesterMatrix(SetRotationMatrixFrom3f(tmp),4);
       this.Transform.elements[3][3] = this.zoomScale;
    },

    // Rotates to a position (4 element vector)
    rotateTo: function(pos)
    {
        this.LastRot   = [  1.0,  0.0,  0.0,                  // Last Rotation
                           0.0,  1.0,  0.0,
                           0.0,  0.0,  1.0 ];
        var m = $M(this.Transform);
        m = m.inverse();
        var camera_position = $V(this.position.elements.slice(0, 3));//.x(1.0 / this.position.elements[3]);
        var p = camera_position.elements;
        var mag = Math.sqrt(p[0]*p[0] + p[1] * p[1] + p[2] * p[2]);
        p = pos.elements;
        var dst_mag = Math.sqrt(p[0]*p[0] + p[1] * p[1] + p[2] * p[2]);
        var normalized_src = camera_position.toUnitVector();
        var normalized_dst = pos.toUnitVector();
        normalized_src = Vector.create([normalized_src.elements[0], normalized_src.elements[1], normalized_src.elements[2]]);
        normalized_dst = Vector.create([normalized_dst.elements[0], normalized_dst.elements[1], normalized_dst.elements[2]]);
        
        //Compute the vector perpendicular to the begin and end vectors
        var Perp = cross(normalized_src.elements, normalized_dst.elements);

        quat = [0.0, 0.0, 0.0, 0.0];
        //Compute the length of the perpendicular vector
        if (Vector3fLength(Perp) > 1.0e-5){//if its non-zero
            //In the quaternion values, w is cosine (theta / 2), where theta is rotation angle
            quat = [Perp[0],Perp[1],Perp[2],dot(normalized_src.elements, normalized_dst.elements)];
        } 

        this.ThisRot = Matrix3fSetRotationFromQuat4f(quat);

        //accumulate the current rotation to all previous rotations
        var tmp = ArrayToSylvesterMatrix(this.ThisRot,3)
                    .x(ArrayToSylvesterMatrix(this.LastRot,3))

        //save rotation for next mouse event
        this.ThisRot = SylvesterToArray(tmp);

        //set the final transform matrix that we will multiply by the modelView
        this.Transform = ArrayToSylvesterMatrix(SetRotationMatrixFrom3f(tmp),4);
        //this.Transform.elements[3][3] *= dst_mag;
        this.position.elements[2] = dst_mag;
        this.zoomScale = this.position.elements[2];
    },
    
    rotateByAngle: function(angle, axis)
    {
        var x = axis == 'x' ? 1 : 0;
        var y = axis == 'y' ? 1 : 0;
        var z = axis == 'z' ? 1 : 0;
        var inRadians = angle * Math.PI / 180.0;
        var rotation = Matrix.Rotation(inRadians, Vector.create([x, y, z])).flatten();
        this.ThisRot = rotation;
        var tmp = ArrayToSylvesterMatrix(this.ThisRot, 3)
                    .x(ArrayToSylvesterMatrix(this.LastRot, 3));
        this.ThisRot = SylvesterToArray(tmp);

        this.Transform = this.Transform.x(ArrayToSylvesterMatrix(SetRotationMatrixFrom3f(tmp), 4));
    }
};
/*-------- End ArcBall*/

