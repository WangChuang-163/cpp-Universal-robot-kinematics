#include <ur_kin.h>

#include <math.h>
#include <stdio.h>

const double damped_max=0.2;
const double damped=0.15;


void matMul16(double mat1[16], double mat2[16], double resMat[16]){	
    resMat[0] = mat1[0]*mat2[0] + mat1[1]*mat2[4] + mat1[2]*mat2[8]  + mat1[3]*mat2[12];
    resMat[1] = mat1[0]*mat2[1] + mat1[1]*mat2[5] + mat1[2]*mat2[9]  + mat1[3]*mat2[13];
    resMat[2] = mat1[0]*mat2[2] + mat1[1]*mat2[6] + mat1[2]*mat2[10] + mat1[3]*mat2[14];
    resMat[3] = mat1[0]*mat2[3] + mat1[1]*mat2[7] + mat1[2]*mat2[11] + mat1[3]*mat2[15];
	
    resMat[4] = mat1[4]*mat2[0] + mat1[5]*mat2[4] + mat1[6]*mat2[8]  + mat1[7]*mat2[12];
    resMat[5] = mat1[4]*mat2[1] + mat1[5]*mat2[5] + mat1[6]*mat2[9]  + mat1[7]*mat2[13];
    resMat[6] = mat1[4]*mat2[2] + mat1[5]*mat2[6] + mat1[6]*mat2[10] + mat1[7]*mat2[14];
    resMat[7] = mat1[4]*mat2[3] + mat1[5]*mat2[7] + mat1[6]*mat2[11] + mat1[7]*mat2[15];
	
    resMat[8] = mat1[8]*mat2[0] + mat1[9]*mat2[4] + mat1[10]*mat2[8]  + mat1[11]*mat2[12];
    resMat[9] = mat1[8]*mat2[1] + mat1[9]*mat2[5] + mat1[10]*mat2[9]  + mat1[11]*mat2[13];
    resMat[10] = mat1[8]*mat2[2] + mat1[9]*mat2[6] + mat1[10]*mat2[10] + mat1[11]*mat2[14];
    resMat[11] = mat1[8]*mat2[3] + mat1[9]*mat2[7] + mat1[10]*mat2[11] + mat1[11]*mat2[15];

    resMat[12] = mat1[12]*mat2[0] + mat1[13]*mat2[4] + mat1[14]*mat2[8]  + mat1[15]*mat2[12];
    resMat[13] = mat1[12]*mat2[1] + mat1[13]*mat2[5] + mat1[14]*mat2[9]  + mat1[15]*mat2[13];
    resMat[14] = mat1[12]*mat2[2] + mat1[13]*mat2[6] + mat1[14]*mat2[10] + mat1[15]*mat2[14];
    resMat[15] = mat1[12]*mat2[3] + mat1[13]*mat2[7] + mat1[14]*mat2[11] + mat1[15]*mat2[15];
}


namespace ur_kinematics {

  namespace {
    const double ZERO_THRESH = 0.00000001;
    int SIGN(double x) {
      return (x > 0) - (x < 0);
    }
    const double PI = M_PI;

   /* //#define UR10_PARAMS
    #ifdef UR10_PARAMS
    const double d1 =  0.1273;
    const double a2 = -0.612;
    const double a3 = -0.5723;
    const double d4 =  0.163941;
    const double d5 =  0.1157;
    const double d6 =  0.0922;
    #endif*/

    //#define UR5_PARAMS
  //  #ifdef UR5_PARAMS
    const double d1 =  0.089159;
    const double a2 = -0.42500;
    const double a3 = -0.39225;
    const double d4 =  0.10915;
    const double d5 =  0.09465;
    const double d6 =  0.0823;
  //  #endif
    
  /*  //#define UR3_PARAMS
    #ifdef UR3_PARAMS
    const double d1 =  0.1519;
    const double a2 = -0.24365;
    const double a3 = -0.21325;
    const double d4 =  0.11235;
    const double d5 =  0.08535;
    const double d6 =  0.0819;
    #endif*/
  }
//参考论文中的DH参考系，即末端坐标系同论文
void forward(const double* q, double T[16]) {
    
   // double T[16];
    double s1 = sin(*q), c1 = cos(*q); q++; // q1
    double q23 = *q, q234 = *q, s2 = sin(*q), c2 = cos(*q); q++; // q2
    double s3 = sin(*q), c3 = cos(*q); q23 += *q; q234 += *q; q++; // q3
    q234 += *q; q++; // q4
    double s5 = sin(*q), c5 = cos(*q); q++; // q5
    double s6 = sin(*q), c6 = cos(*q); // q6
    double s23 = sin(q23), c23 = cos(q23);
    double s234 = sin(q234), c234 = cos(q234);
   /* T[0] = ((c1*c234-s1*s234)*s5)/2.0 - c5*s1 + ((c1*c234+s1*s234)*s5)/2.0;
    T[1] = (c6*(s1*s5 + ((c1*c234-s1*s234)*c5)/2.0 + ((c1*c234+s1*s234)*c5)/2.0) - 
          (s6*((s1*c234+c1*s234) - (s1*c234-c1*s234)))/2.0);
    T[2] = (-(c6*((s1*c234+c1*s234) - (s1*c234-c1*s234)))/2.0 - 
          s6*(s1*s5 + ((c1*c234-s1*s234)*c5)/2.0 + ((c1*c234+s1*s234)*c5)/2.0));
    T[3] = ((d5*(s1*c234-c1*s234))/2.0 - (d5*(s1*c234+c1*s234))/2.0 - 
          d4*s1 + (d6*(c1*c234-s1*s234)*s5)/2.0 + (d6*(c1*c234+s1*s234)*s5)/2.0 - 
          a2*c1*c2 - d6*c5*s1 - a3*c1*c2*c3 + a3*c1*s2*s3);
    T[4] = c1*c5 + ((s1*c234+c1*s234)*s5)/2.0 + ((s1*c234-c1*s234)*s5)/2.0;
    T[5] = (c6*(((s1*c234+c1*s234)*c5)/2.0 - c1*s5 + ((s1*c234-c1*s234)*c5)/2.0) + 
          s6*((c1*c234-s1*s234)/2.0 - (c1*c234+s1*s234)/2.0));
    T[6] = (c6*((c1*c234-s1*s234)/2.0 - (c1*c234+s1*s234)/2.0) - 
          s6*(((s1*c234+c1*s234)*c5)/2.0 - c1*s5 + ((s1*c234-c1*s234)*c5)/2.0));
    T[7] = ((d5*(c1*c234-s1*s234))/2.0 - (d5*(c1*c234+s1*s234))/2.0 + d4*c1 + 
          (d6*(s1*c234+c1*s234)*s5)/2.0 + (d6*(s1*c234-c1*s234)*s5)/2.0 + d6*c1*c5 - 
          a2*c2*s1 - a3*c2*c3*s1 + a3*s1*s2*s3);
    T[8] = ((c234*c5-s234*s5)/2.0 - (c234*c5+s234*s5)/2.0);
    T[9] = ((s234*c6-c234*s6)/2.0 - (s234*c6+c234*s6)/2.0 - s234*c5*c6);
    T[10] = (s234*c5*s6 - (c234*c6+s234*s6)/2.0 - (c234*c6-s234*s6)/2.0);
    T[11] = (d1 + (d6*(c234*c5-s234*s5))/2.0 + a3*(s2*c3+c2*s3) + a2*s2 - 
         (d6*(c234*c5+s234*s5))/2.0 - d5*c234);
    T[12] = 0.0; T[13] = 0.0; T[14] = 0.0; T[15] = 1.0;*/

    T[0]= c6*(s1*s5 + c234*c1*c5) - s234*c1*s6;
    T[1]= - s6*(s1*s5 + c234*c1*c5) - s234*c1*c6;
    T[2]= c5*s1 - c234*c1*s5;
    T[3]= d6*(c5*s1 - c234*c1*s5) + c1*(a3*c23 + a2*c2) + d4*s1 + d5*s234*c1;
    T[4]= - c6*(c1*s5 - c234*c5*s1) - s234*s1*s6;
    T[5]=  s6*(c1*s5 - c234*c5*s1) - s234*c6*s1;
    T[6]= - c1*c5 - c234*s1*s5;
    T[7]= s1*(a3*c23 + a2*c2) - d4*c1 - d6*(c1*c5 + c234*s1*s5) + d5*s234*s1;
    T[8]= c234*s6 + s234*c5*c6;
    T[9]= c234*c6 - s234*c5*s6;
    T[10]=-s234*s5;
    T[11]=d1 + a3*s23 + a2*s2 - d5*c234 - d6*s234*s5;
    T[12]=0.0; T[13]=0.0;T[14]=0.0;T[15]=1.0;
   
    /*double T1[16];
    T1[0] = -1;	T1[1] = 0;   T1[2] = 0;	 T1[3] = 0;
    T1[4] = 0;	T1[5] =	-1;  T1[6] = 0;	 T1[7] = 0;
    T1[8] = 0;	T1[9] =	0;   T1[10] = 1; T1[11] = 0;
    T1[12] = 0;	T1[13] = 0;  T1[14] = 0; T1[15] = 1;
  
    double T2[16];
    T2[0] = 0;	T2[1] = 0;  T2[2] = 1;	T2[3] = 0;
    T2[4] = -1;	T2[5] =	0;  T2[6] = 0;	T2[7] = 0;
    T2[8] = 0;	T2[9] =	-1; T2[10] = 0; T2[11] = 0;
    T2[12] = 0;	T2[13] = 0; T2[14] = 0; T2[15] = 1;
  
    double tmp[16];
    matMul16(T1,T,tmp);
    matMul16(tmp,T2,T06);*/
}


  void forward_all(const double* q, double* T1, double* T2, double* T3, 
                                    double* T4, double* T5, double* T6) {
    double s1 = sin(*q), c1 = cos(*q); q++; // q1
    double q23 = *q, q234 = *q, s2 = sin(*q), c2 = cos(*q); q++; // q2
    double s3 = sin(*q), c3 = cos(*q); q23 += *q; q234 += *q; q++; // q3
    q234 += *q; q++; // q4
    double s5 = sin(*q), c5 = cos(*q); q++; // q5
    double s6 = sin(*q), c6 = cos(*q); // q6
    double s23 = sin(q23), c23 = cos(q23);
    double s234 = sin(q234), c234 = cos(q234);

    if(T1 != NULL) {
      *T1 = c1; T1++;
      *T1 = 0; T1++;
      *T1 = s1; T1++;
      *T1 = 0; T1++;
      *T1 = s1; T1++;
      *T1 = 0; T1++;
      *T1 = -c1; T1++;
      *T1 = 0; T1++;
      *T1 =       0; T1++;
      *T1 = 1; T1++;
      *T1 = 0; T1++;
      *T1 =d1; T1++;
      *T1 =       0; T1++;
      *T1 = 0; T1++;
      *T1 = 0; T1++;
      *T1 = 1; T1++;
    }

    if(T2 != NULL) {
      *T2 = c1*c2; T2++;
      *T2 = -c1*s2; T2++;
      *T2 = s1; T2++;
      *T2 =a2*c1*c2; T2++;
      *T2 = c2*s1; T2++;
      *T2 = -s1*s2; T2++;
      *T2 = -c1; T2++;
      *T2 =a2*c2*s1; T2++;
      *T2 =         s2; T2++;
      *T2 = c2; T2++;
      *T2 = 0; T2++;
      *T2 =   d1 + a2*s2; T2++;
      *T2 =               0; T2++;
      *T2 = 0; T2++;
      *T2 = 0; T2++;
      *T2 =                 1; T2++;
    }

    if(T3 != NULL) {
      *T3 = c23*c1; T3++;
      *T3 = -s23*c1; T3++;
      *T3 = s1; T3++;
      *T3 =c1*(a3*c23 + a2*c2); T3++;
      *T3 = c23*s1; T3++;
      *T3 = -s23*s1; T3++;
      *T3 = -c1; T3++;
      *T3 =s1*(a3*c23 + a2*c2); T3++;
      *T3 =         s23; T3++;
      *T3 = c23; T3++;
      *T3 = 0; T3++;
      *T3 =     d1 + a3*s23 + a2*s2; T3++;
      *T3 =                    0; T3++;
      *T3 = 0; T3++;
      *T3 = 0; T3++;
      *T3 =                                     1; T3++;
    }

    if(T4 != NULL) {
      *T4 = c234*c1; T4++;
      *T4 = s1; T4++;
      *T4 = s234*c1; T4++;
      *T4 =c1*(a3*c23 + a2*c2) + d4*s1; T4++;
      *T4 = c234*s1; T4++;
      *T4 = -c1; T4++;
      *T4 = s234*s1; T4++;
      *T4 =s1*(a3*c23 + a2*c2) - d4*c1; T4++;
      *T4 =         s234; T4++;
      *T4 = 0; T4++;
      *T4 = -c234; T4++;
      *T4 =                  d1 + a3*s23 + a2*s2; T4++;
      *T4 =                         0; T4++;
      *T4 = 0; T4++;
      *T4 = 0; T4++;
      *T4 =                                                  1; T4++;
    }

    if(T5 != NULL) {
      *T5 = s1*s5 + c234*c1*c5; T5++;
      *T5 = -s234*c1; T5++;
      *T5 = c5*s1 - c234*c1*s5; T5++;
      *T5 =c1*(a3*c23 + a2*c2) + d4*s1 + d5*s234*c1; T5++;
      *T5 = c234*c5*s1 - c1*s5; T5++;
      *T5 = -s234*s1; T5++;
      *T5 = - c1*c5 - c234*s1*s5; T5++;
      *T5 =s1*(a3*c23 + a2*c2) - d4*c1 + d5*s234*s1; T5++;
      *T5 =                           s234*c5; T5++;
      *T5 = c234; T5++;
      *T5 = -s234*s5; T5++;
      *T5 =                          d1 + a3*s23 + a2*s2 - d5*c234; T5++;
      *T5 =                                                   0; T5++;
      *T5 = 0; T5++;
      *T5 = 0; T5++;
      *T5 =                                                                                 1; T5++;
    }

    if(T6 != NULL) {
      *T6 =   c6*(s1*s5 + c234*c1*c5) - s234*c1*s6; T6++;
      *T6 = - s6*(s1*s5 + c234*c1*c5) - s234*c1*c6; T6++;
      *T6 = c5*s1 - c234*c1*s5; T6++;
      *T6 =d6*(c5*s1 - c234*c1*s5) + c1*(a3*c23 + a2*c2) + d4*s1 + d5*s234*c1; T6++;
      *T6 = - c6*(c1*s5 - c234*c5*s1) - s234*s1*s6; T6++;
      *T6 = s6*(c1*s5 - c234*c5*s1) - s234*c6*s1; T6++;
      *T6 = - c1*c5 - c234*s1*s5; T6++;
      *T6 =s1*(a3*c23 + a2*c2) - d4*c1 - d6*(c1*c5 + c234*s1*s5) + d5*s234*s1; T6++;
      *T6 =                                       c234*s6 + s234*c5*c6; T6++;
      *T6 = c234*c6 - s234*c5*s6; T6++;
      *T6 = -s234*s5; T6++;
      *T6 =                                                      d1 + a3*s23 + a2*s2 - d5*c234 - d6*s234*s5; T6++;
      *T6 =                                                                                                   0; T6++;
      *T6 = 0; T6++;
      *T6 = 0; T6++;
      *T6 =                                                                                                                                            1; T6++;
    }
  }

  int inverse(const double* T, double* q_sols, double q6_des) {
    int num_sols = 0;
    double T02 = -*T; T++; double T00 =  *T; T++; double T01 =  *T; T++; double T03 = -*T; T++; 
    double T12 = -*T; T++; double T10 =  *T; T++; double T11 =  *T; T++; double T13 = -*T; T++; 
    double T22 =  *T; T++; double T20 = -*T; T++; double T21 = -*T; T++; double T23 =  *T;

   /* double T00 =  *T; T++; double T01 =  *T; T++; double T02 = *T; T++; double T03 = *T; T++; 
    double T10 =  *T; T++; double T11 =  *T; T++; double T12 = *T; T++; double T13 = *T; T++; 
    double T20 =  *T; T++; double T21 =  *T; T++; double T22 = *T; T++; double T23 =  *T;*/

    ////////////////////////////// shoulder rotate joint (q1) //////////////////////////////
    double q1[2];
    {
      double A = d6*T12 - T13;
      double B = d6*T02 - T03;
      double R = A*A + B*B;
      if(fabs(A) < ZERO_THRESH) {
        double div;
        if(fabs(fabs(d4) - fabs(B)) < ZERO_THRESH)
          div = -SIGN(d4)*SIGN(B);
        else
          div = -d4/B;
        double arcsin = asin(div);
        if(fabs(arcsin) < ZERO_THRESH)
          arcsin = 0.0;
        if(arcsin < 0.0)
          q1[0] = arcsin + 2.0*PI;
        else
          q1[0] = arcsin;
        q1[1] = PI - arcsin;
      }
      else if(fabs(B) < ZERO_THRESH) {
        double div;
        if(fabs(fabs(d4) - fabs(A)) < ZERO_THRESH)
          div = SIGN(d4)*SIGN(A);
        else
          div = d4/A;
        double arccos = acos(div);
        q1[0] = arccos;
        q1[1] = 2.0*PI - arccos;
      }
      else if(d4*d4 > R) {
        return num_sols;
      }
      else {
        double arccos = acos(d4 / sqrt(R)) ;
        double arctan = atan2(-B, A);
        double pos = arccos + arctan;
        double neg = -arccos + arctan;
        if(fabs(pos) < ZERO_THRESH)
          pos = 0.0;
        if(fabs(neg) < ZERO_THRESH)
          neg = 0.0;
        if(pos >= 0.0)
          q1[0] = pos;
        else
          q1[0] = 2.0*PI + pos;
        if(neg >= 0.0)
          q1[1] = neg; 
        else
          q1[1] = 2.0*PI + neg;
      }
    }
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////// wrist 2 joint (q5) //////////////////////////////
    double q5[2][2];
    {
      for(int i=0;i<2;i++) {
        double numer = (T03*sin(q1[i]) - T13*cos(q1[i])-d4);
        double div;
        if(fabs(fabs(numer) - fabs(d6)) < ZERO_THRESH)
          div = SIGN(numer) * SIGN(d6);
        else
          div = numer / d6;
        double arccos = acos(div);
        q5[i][0] = arccos;
        q5[i][1] = 2.0*PI - arccos;
      }
    }
    ////////////////////////////////////////////////////////////////////////////////

    {
      for(int i=0;i<2;i++) {
        for(int j=0;j<2;j++) {
          double c1 = cos(q1[i]), s1 = sin(q1[i]);
          double c5 = cos(q5[i][j]), s5 = sin(q5[i][j]);
          double q6;
          ////////////////////////////// wrist 3 joint (q6) //////////////////////////////
          if(fabs(s5) < ZERO_THRESH)
            q6 = q6_des;
          else {
            q6 = atan2(SIGN(s5)*-(T01*s1 - T11*c1), 
                       SIGN(s5)*(T00*s1 - T10*c1));
            if(fabs(q6) < ZERO_THRESH)
              q6 = 0.0;
            if(q6 < 0.0)
              q6 += 2.0*PI;
          }
          ////////////////////////////////////////////////////////////////////////////////

          double q2[2], q3[2], q4[2];
          ///////////////////////////// RRR joints (q2,q3,q4) ////////////////////////////
          double c6 = cos(q6), s6 = sin(q6);
          double x04x = -s5*(T02*c1 + T12*s1) - c5*(s6*(T01*c1 + T11*s1) - c6*(T00*c1 + T10*s1));
          double x04y = c5*(T20*c6 - T21*s6) - T22*s5;
          double p13x = d5*(s6*(T00*c1 + T10*s1) + c6*(T01*c1 + T11*s1)) - d6*(T02*c1 + T12*s1) + 
                        T03*c1 + T13*s1;
          double p13y = T23 - d1 - d6*T22 + d5*(T21*c6 + T20*s6);

          double c3 = (p13x*p13x + p13y*p13y - a2*a2 - a3*a3) / (2.0*a2*a3);
          if(fabs(fabs(c3) - 1.0) < ZERO_THRESH)
            c3 = SIGN(c3);
          else if(fabs(c3) > 1.0) {
            // TODO NO SOLUTION
            continue;
          }
          double arccos = acos(c3);
          q3[0] = arccos;
          q3[1] = 2.0*PI - arccos;
          double denom = a2*a2 + a3*a3 + 2*a2*a3*c3;
          double s3 = sin(arccos);
          double A = (a2 + a3*c3), B = a3*s3;
          q2[0] = atan2((A*p13y - B*p13x) / denom, (A*p13x + B*p13y) / denom);
          q2[1] = atan2((A*p13y + B*p13x) / denom, (A*p13x - B*p13y) / denom);
          double c23_0 = cos(q2[0]+q3[0]);
          double s23_0 = sin(q2[0]+q3[0]);
          double c23_1 = cos(q2[1]+q3[1]);
          double s23_1 = sin(q2[1]+q3[1]);
          q4[0] = atan2(c23_0*x04y - s23_0*x04x, x04x*c23_0 + x04y*s23_0);
          q4[1] = atan2(c23_1*x04y - s23_1*x04x, x04x*c23_1 + x04y*s23_1);
          ////////////////////////////////////////////////////////////////////////////////
          for(int k=0;k<2;k++) {
            if(fabs(q2[k]) < ZERO_THRESH)
              q2[k] = 0.0;
            else if(q2[k] < 0.0) q2[k] += 2.0*PI;
            if(fabs(q4[k]) < ZERO_THRESH)
              q4[k] = 0.0;
            else if(q4[k] < 0.0) q4[k] += 2.0*PI;
            q_sols[num_sols*6+0] = q1[i];    q_sols[num_sols*6+1] = q2[k]; 
            q_sols[num_sols*6+2] = q3[k];    q_sols[num_sols*6+3] = q4[k]; 
            q_sols[num_sols*6+4] = q5[i][j]; q_sols[num_sols*6+5] = q6; 
            num_sols++;
          }

        }
      }
    }
    return num_sols;
  }
/*
void jacobian(double *q,double J[36])
{
    //double T[16];
    double s1 = sin(*q), c1 = cos(*q); q++; // q1
    double q23 = *q, q234 = *q, s2 = sin(*q), c2 = cos(*q); q++; // q2
    double s3 = sin(*q), c3 = cos(*q); q23 += *q; q234 += *q; q++; // q3
    q234 += *q; q++; // q4
    double s5 = sin(*q), c5 = cos(*q); q++; // q5
    double s6 = sin(*q), c6 = cos(*q); // q6
    double s23 = sin(q23), c23 = cos(q23);
    double s234 = sin(q234), c234 = cos(q234);
    double p4x=c1*(a3*c23+a2*c2)+d4*s1; double p4y=s1*(a3*c23+a2*c2)-d4*c1; double p4z=d1+a3*s23+a2*s2;
    double p5x=c1*(a3*c23+a2*c2)+d4*s1+d5*s234*c1; double p5y=s1*(a3*c23+a2*c2)-d4*c1+d5*s234*s1; double p5z=d1+a3*s23+a2*s2-d5*c234;
    double p6x=d6*(c5*s1-c234*c1*s5)+c1*(a3*c23+a2*c2)+d4*s1+d5*s234*c1;
    double p6y=s1*(a3*c23+a2*c2)-d4*c1-d6*(c1*c5+c234*s1*s5)+d5*s234*s1;
    double p6z=d1+a3*s23+a2*s2-d5*c234-d6*s234*s5;
    double z5x=c5*s1-c234*c1*s5; double z5y=-c1*c5-c234*s1*s5; double z5z=-s234*s5;
  
    J[0]=-p6y;
    J[1]=-c1*(p6z-d1);
    J[2]=-c1*(p6z-d1-a2*s2);
    J[3]=-c1*(p6z-d1-a3*s23-a2*s2);
    J[4]=s234*s1*(p6z-p4z)+c234*(p6y-p4y);
    J[5]=z5y*(p6z-p5z)-z5z*(p6y-p5y);
    
    J[6]=p6x;
    J[7]=-s1*(p6z-d1);
    J[8]=-s1*(p6z-d1-a2*s2);
    J[9]=-s1*(p6z-d1-a3*s23-a2*s2);
    J[10]=-c234*(p6x-p4x)-s234*c1*(p6z-p4z);
    J[11]=z5z*(p6x-p5x)-z5x*(p6z-p5z);

    J[12]=0;
    J[13]=c1*p6x+s1*p6y;
    J[14]=c1*(p6x-a2*c1*c2)+s1*(p6y-a2*c2*s1);
    J[15]=c1*(p6x-c1*(a3*c23+a2*c2))+s1*(p6y-s1*(a3*c23+a2*c2));
    J[16]=s234*c1*(p6y-p4y)-s234*s1*(p6x-p4x);
    J[17]=z5x*(p6y-p5y)-z5y*(p6x-p5x);

    J[18]=0;
    J[19]=s1;
    J[20]=s1;
    J[21]=s1;
    J[22]=s234*c1;
    J[23]=z5x;

    J[24]=0;
    J[25]=-c1;
    J[26]=-c1;
    J[27]=-c1;
    J[28]=s234*s1;
    J[29]=z5y;

    J[30]=1;
    J[31]=0;
    J[32]=0;
    J[33]=0;
    J[34]=-c234;
    J[35]=z5z;
}*/

void jacobian(Vector6d &q,Matrix6d &J)
{
    //double T[16];
    double s1 = sin(q(0)), c1 = cos(q(0));  // q1
    double q23 = q(1), q234 = q(1), s2 = sin(q(1)), c2 = cos(q(1));  // q2
    double s3 = sin(q(2)), c3 = cos(q(2)); q23 += q(2); q234 += q(2); // q3
    q234 += q(3);  // q4
    double s5 = sin(q(4)), c5 = cos(q(4));  // q5
    double s6 = sin(q(5)), c6 = cos(q(5)); // q6
    double s23 = sin(q23), c23 = cos(q23);
    double s234 = sin(q234), c234 = cos(q234);
    double p4x=c1*(a3*c23+a2*c2)+d4*s1; double p4y=s1*(a3*c23+a2*c2)-d4*c1; double p4z=d1+a3*s23+a2*s2;
    double p5x=c1*(a3*c23+a2*c2)+d4*s1+d5*s234*c1; double p5y=s1*(a3*c23+a2*c2)-d4*c1+d5*s234*s1; double p5z=d1+a3*s23+a2*s2-d5*c234;
    double p6x=d6*(c5*s1-c234*c1*s5)+c1*(a3*c23+a2*c2)+d4*s1+d5*s234*c1;
    double p6y=s1*(a3*c23+a2*c2)-d4*c1-d6*(c1*c5+c234*s1*s5)+d5*s234*s1;
    double p6z=d1+a3*s23+a2*s2-d5*c234-d6*s234*s5;
    double z5x=c5*s1-c234*c1*s5; double z5y=-c1*c5-c234*s1*s5; double z5z=-s234*s5;
  
    J(0,0)=-p6y;
    J(0,1)=-c1*(p6z-d1);
    J(0,2)=-c1*(p6z-d1-a2*s2);
    J(0,3)=-c1*(p6z-d1-a3*s23-a2*s2);
    J(0,4)=s234*s1*(p6z-p4z)+c234*(p6y-p4y);
    J(0,5)=z5y*(p6z-p5z)-z5z*(p6y-p5y);
    
    J(1,0)=p6x;
    J(1,1)=-s1*(p6z-d1);
    J(1,2)=-s1*(p6z-d1-a2*s2);
    J(1,3)=-s1*(p6z-d1-a3*s23-a2*s2);
    J(1,4)=-c234*(p6x-p4x)-s234*c1*(p6z-p4z);
    J(1,5)=z5z*(p6x-p5x)-z5x*(p6z-p5z);

    J(2,0)=0;
    J(2,1)=c1*p6x+s1*p6y;
    J(2,2)=c1*(p6x-a2*c1*c2)+s1*(p6y-a2*c2*s1);
    J(2,3)=c1*(p6x-c1*(a3*c23+a2*c2))+s1*(p6y-s1*(a3*c23+a2*c2));
    J(2,4)=s234*c1*(p6y-p4y)-s234*s1*(p6x-p4x);
    J(2,5)=z5x*(p6y-p5y)-z5y*(p6x-p5x);

    J(3,0)=0;
    J(3,1)=s1;
    J(3,2)=s1;
    J(3,3)=s1;
    J(3,4)=s234*c1;
    J(3,5)=z5x;

    J(4,0)=0;
    J(4,1)=-c1;
    J(4,2)=-c1;
    J(4,3)=-c1;
    J(4,4)=s234*s1;
    J(4,5)=z5y;

    J(5,0)=1;
    J(5,1)=0;
    J(5,2)=0;
    J(5,3)=0;
    J(5,4)=-c234;
    J(5,5)=z5z;
}

Vector6d DLS_inverse(Vector6d q,Vector6d v_mat)
{   
    Matrix6d DLS_J;
    DLS_J.Identity();
    ur_kinematics::jacobian(q,DLS_J);

    Vector6d qq_mat;

    //BDCSVD<Matrix6d> svd(DLS_J,ComputeFullU | ComputeFullV);
    JacobiSVD<Matrix6d> svd(DLS_J,ComputeFullU | ComputeFullV);
   // JacobiSVD<Matrix6d> svd(DLS_J);
    Vector6d singlar_mat=svd.singularValues();
    //std::cout<<singlar_mat<<std::endl;
    MatrixXd V=svd.matrixV();
    MatrixXd U=svd.matrixU();
    U.transposeInPlace();

    double singular_min=singlar_mat.minCoeff();
    //double singular_min=singlar_mat(5);
    //ROS_INFO("The min singular is: %f",singular_min);
    if (singular_min>=damped)
    {
        qq_mat=(1/singlar_mat(0))*V.col(0)*U.row(0)*v_mat+(1/singlar_mat(1))*V.col(1)*U.row(1)*v_mat+
        (1/singlar_mat(2))*V.col(2)*U.row(2)*v_mat+(1/singlar_mat(3))*V.col(3)*U.row(3)*v_mat+
        (1/singlar_mat(4))*V.col(4)*U.row(4)*v_mat+(1/singlar_mat(5))*V.col(5)*U.row(5)*v_mat;
         //qq_mat=DLS_J.inverse()*v_mat;
    }
    else
    {
       double damped_factor=(1-(singular_min/damped)*(singular_min/damped))*damped_max*damped_max;
       qq_mat=(singlar_mat(0)/(singlar_mat(0)*singlar_mat(0)+damped_factor*damped_factor))*V.col(0)*U.row(0)*v_mat+
       (singlar_mat(1)/(singlar_mat(1)*singlar_mat(1)+damped_factor*damped_factor))*V.col(1)*U.row(1)*v_mat+
       (singlar_mat(2)/(singlar_mat(2)*singlar_mat(2)+damped_factor*damped_factor))*V.col(2)*U.row(2)*v_mat+
       (singlar_mat(3)/(singlar_mat(3)*singlar_mat(3)+damped_factor*damped_factor))*V.col(3)*U.row(3)*v_mat+
       (singlar_mat(4)/(singlar_mat(4)*singlar_mat(4)+damped_factor*damped_factor))*V.col(4)*U.row(4)*v_mat+
       (singlar_mat(5)/(singlar_mat(5)*singlar_mat(5)+damped_factor*damped_factor))*V.col(5)*U.row(5)*v_mat;
    }

    return qq_mat;
   // delete [] J;
}

Vector6d DLS_inverse2(Vector6d q,Vector6d v_mat)
{
    Matrix6d DLS_J;
    DLS_J.Identity();
    ur_kinematics::jacobian(q,DLS_J);

    Vector6d qq_mat;
    qq_mat=DLS_J.inverse()*v_mat;
    return qq_mat;
}

};

