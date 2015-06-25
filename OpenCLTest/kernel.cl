#define M_PI 3.141592653589793

float d_u(float x, float y, float t){
    return (-2.0f*native_cos(M_PI*(t)/8.0f)*native_sin(M_PI*(x))*native_sin(M_PI*(x))*native_cos(M_PI*(y))*native_sin(M_PI*(y)));
}

float d_v(float x, float y, float t){
    return (2.0f*native_cos(M_PI*(t)/8.0f)*native_cos(M_PI*(x))*native_sin(M_PI*(x))*native_sin(M_PI*(y))*native_sin(M_PI*(y)));
}

__kernel void d_rungekutta(int num_particles, __global float3 *position, float a_time, float a_dt){
    int index;
    float prex,prey,p1,q1,p2,q2,p3,q3,p4,q4;
    float x,y,t;
    index = get_global_id(0);
    if(index<num_particles){
        prex=position[index].x;
        prey=position[index].y;
        
        p1=d_u(prex,prey,a_time);
        q1=d_v(prex,prey,a_time);
        
        x=prex+0.5f*p1*a_dt;
        y=prey+0.5f*q1*a_dt;
        t=a_time+0.5f*a_dt;
        p2=d_u(x,y,t);
        q2=d_v(x,y,t);
        
        
        x=prex+0.5f*p2*a_dt;
        y=prey+0.5f*q2*a_dt;
        t=a_time+0.5f*a_dt;
        p3=d_u(x,y,t);
        q3=d_v(x,y,t);
        
        
        x=prex+p3*a_dt;
        y=prey+q3*a_dt;
        t=a_time+a_dt;
        p4=d_u(x,y,t);
        q4=d_v(x,y,t);
        
        position[index].x=prex+(p1+2.0f*p2+2.0f*p3+p4)/6.0f*a_dt;
        position[index].y=prey+(q1+2.0f*q2+2.0f*q3+q4)/6.0f*a_dt;
        
        
    }
}