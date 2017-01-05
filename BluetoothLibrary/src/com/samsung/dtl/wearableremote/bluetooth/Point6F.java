package com.samsung.dtl.wearableremote.bluetooth;

import android.graphics.Point;
import android.os.Parcel;
import android.os.Parcelable;
import android.util.FloatMath;


/**
 * PointF holds two float coordinates
 */
public class Point6F implements Parcelable {
    public float x;
    public float y;
    public float z;
    public float yaw;
    public float pitch;
    public float roll;
    
    public Point6F() {}

    public Point6F(float x, float y, float z, float yaw, float pitch, float roll) {
        this.x = x;
        this.y = y;
        this.z = z; 
        this.yaw = yaw;
        this.pitch = pitch;
        this.roll = roll;
    }
    
    public Point6F(Point p) { 
        this.x = p.x;
        this.y = p.y;
        this.y = 1;
        this.yaw = 0;
        this.pitch = 0;
        this.roll = 0;
    }
    
    /**
     * Set the point's x and y coordinates
     */
    public final void set(float x, float y, float z, float yaw, float pitch, float roll) {
        this.x = x;
        this.y = y;
        this.z = z;
        this.yaw = yaw;
        this.pitch = pitch;
        this.roll = roll;
    }
    
    /**
     * Set the point's x and y coordinates to the coordinates of p
     */
    public final void set(Point6F p) { 
        this.x = p.x;
        this.y = p.y;
        this.z = p.z;
        this.yaw = p.yaw;
        this.pitch = p.pitch;
        this.roll = p.roll;
    }
    
    public final void negate() { 
        x = -x;
        y = -y;
        z = -z;
    }
    
    public final void offset(float dx, float dy, float dz) {
        x += dx;
        y += dy;
        z += dz;
    }
    
    /**
     * Returns true if the point's coordinates equal (x,y)
     */
    public final boolean equals(float x, float y, float z) { 
        return (this.x == x && this.y == y ) && this.z == z; 
    }

    /**
     * Return the euclidian distance from (0,0) to the point
     */
    public final float length() { 
        return length(x, y, z); 
    }
    
    /**
     * Returns the euclidian distance from (0,0) to (x,y)
     */
    public static float length(float x, float y, float z) {
        return FloatMath.sqrt(x * x + y * y + z*z);
    }

    /**
     * Parcelable interface methods
     */
    @Override
    public int describeContents() {
        return 0;
    }

    /**
     * Write this point to the specified parcel. To restore a point from
     * a parcel, use readFromParcel()
     * @param out The parcel to write the point's coordinates into
     */
    @Override
    public void writeToParcel(Parcel out, int flags) {
        out.writeFloat(x);
        out.writeFloat(y);
        out.writeFloat(z);
        out.writeFloat(yaw);
        out.writeFloat(pitch);
        out.writeFloat(roll);
    }

    public static final Parcelable.Creator<Point6F> CREATOR = new Parcelable.Creator<Point6F>() {
        /**
         * Return a new point from the data in the specified parcel.
         */
        public Point6F createFromParcel(Parcel in) {
            Point6F r = new Point6F();
            r.readFromParcel(in);
            return r;
        }

        /**
         * Return an array of rectangles of the specified size.
         */
        public Point6F[] newArray(int size) {
            return new Point6F[size];
        }
    };

    /**
     * Set the point's coordinates from the data stored in the specified
     * parcel. To write a point to a parcel, call writeToParcel().
     *
     * @param in The parcel to read the point's coordinates from
     */
    public void readFromParcel(Parcel in) {
        x = in.readFloat();
        y = in.readFloat();
        z = in.readFloat();
        yaw = in.readFloat();
        pitch = in.readFloat();
        roll = in.readFloat();
    }
}
