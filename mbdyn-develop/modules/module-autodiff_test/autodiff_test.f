      SUBROUTINE DEF_JOINT_AD_ASS_RES(X1, g1, XP1, gP1, R1_0, W1_0, 
     $ X2, g2, XP2, gP2, R2_0, W2_0, S1, D1, o1, o2, F1, M1, F2, M2)
      IMPLICIT NONE
      DOUBLE PRECISION X1(3), g1(3), XP1(3), gP1(3), R1_0(3, 3), W1_0(3)
      DOUBLE PRECISION X2(3), g2(3), XP2(3), gP2(3), R2_0(3, 3), W2_0(3)
      DOUBLE PRECISION S1(3, 3), D1(3, 3), o1(3), o2(3)
      DOUBLE PRECISION R1(3, 3), W1(3)
      DOUBLE PRECISION R2(3, 3), W2(3)
      DOUBLE PRECISION R1o1(3), R2o2(3)
      DOUBLE PRECISION dXR1(3), dXPR1(3), dX(3), dXP(3)
      DOUBLE PRECISION W1xR1o1(3), W2xR2o2(3)
      DOUBLE PRECISION F1R1(3), F1(3), M1(3), F2(3), M2(3)
      INTEGER I, J
      EXTERNAL UPDATE_ROT
      
      CALL UPDATE_ROT(g1, gP1, R1_0, W1_0, R1, W1)
      CALL UPDATE_ROT(g2, gP2, R2_0, W2_0, R2, W2)
      
      DO I=1, 3
        R1o1(I) = 0D0
        R2o2(I) = 0D0
        DO J=1, 3
            R1o1(I) = R1o1(I) + R1(I, J) * o1(J)
            R2o2(I) = R2o2(I) + R2(I, J) * o2(J)
        END DO
      END DO
          
      W1xR1o1(1) = W1(2)*R1o1(3)-R1o1(2)*W1(3)
      W1xR1o1(2) = R1o1(1)*W1(3)-W1(1)*R1o1(3)
      W1xR1o1(3) = W1(1)*R1o1(2)-R1o1(1)*W1(2)
      
      W2xR2o2(1) = W2(2)*R2o2(3)-R2o2(2)*W2(3)
      W2xR2o2(2) = R2o2(1)*W2(3)-W2(1)*R2o2(3)
      W2xR2o2(3) = W2(1)*R2o2(2)-R2o2(1)*W2(2)
    
      DO I=1, 3
        dXR1(I) = X1(I) + R1o1(I) - X2(I) - R2o2(I)
        dXPR1(I) = XP1(I) + W1xR1o1(I) - XP2(I) - W2xR2o2(I)
      END DO
      
      DO I=1, 3
        dX(I) = 0D0
        dXP(I) = 0D0
        DO J=1, 3
            dX(I) = dX(I) + R1(J, I) * dXR1(J)
            dXP(I) = dXP(I) + R1(J, I) * dXPR1(J)
        END DO
      END DO
      
      DO I=1, 3
        F1R1(I) = 0D0
        DO J=1, 3
            F1R1(I) = F1R1(I) + S1(I, J) * dX(J) + D1(I, J) * dXP(J)
        END DO
      END DO
      
      DO I=1, 3
        F1(I) = 0D0
        DO J=1, 3
            F1(I) = F1(I) - R1(I, J) * F1R1(J)
        END DO
        F2(I) = -F1(I)
      END DO
      
      M1(1) = R1o1(2)*F1(3)-F1(2)*R1o1(3)
      M1(2) = F1(1)*R1o1(3)-R1o1(1)*F1(3)
      M1(3) = R1o1(1)*F1(2)-F1(1)*R1o1(2)
      
      M2(1) = R2o2(2)*F2(3)-F2(2)*R2o2(3)
      M2(2) = F2(1)*R2o2(3)-R2o2(1)*F2(3)
      M2(3) = R2o2(1)*F2(2)-F2(1)*R2o2(2)
      END
      
      SUBROUTINE UPDATE_ROT(g, gP, R_0, W_0, R, W)
      IMPLICIT NONE
      DOUBLE PRECISION g(3), gP(3), R_0(3, 3), W_0(3)
      DOUBLE PRECISION R(3, 3), W(3)
      DOUBLE PRECISION RDelta(3, 3), Gr(3, 3), d
      DOUBLE PRECISION tmp1, tmp2, tmp3, tmp4, tmp5, tmp6
      DOUBLE PRECISION tmp7, tmp8, tmp9
      INTEGER I, J, K
      
      tmp1 = g(3)**2
      tmp2 = g(2)**2
      tmp3 = g(1)**2
      tmp4 = g(1)*g(2)*0.5D0
      tmp5 = g(2)*g(3)*0.5D0
      tmp6 = g(1)*g(3)*0.5D0
      
      d=4D0/(tmp1+tmp2+tmp3+4D0)
      
      RDelta(1,1) = (-tmp1-tmp2)*d*0.5D0+1D0
      RDelta(1,2) = (tmp4-g(3))*d
      RDelta(1,3) = (tmp6+g(2))*d
      RDelta(2,1) = (g(3)+tmp4)*d
      RDelta(2,2) = (-tmp1-tmp3)*d*0.5D0+1D0
      RDelta(2,3) = (tmp5-g(1))*d
      RDelta(3,1) = (tmp6-g(2))*d
      RDelta(3,2) = (tmp5+g(1))*d
      RDelta(3,3) = (-tmp2-tmp3)*d*0.5D0+1D0
      
      tmp7 = 0.5D0*g(3)*d
      tmp8 = 0.5D0*g(2)*d
      tmp9 = 0.5D0*g(1)*d
      
      Gr(1,1) = d
      Gr(1,2) = -tmp7
      Gr(1,3) = tmp8
      Gr(2,1) = tmp7
      Gr(2,2) = d
      Gr(2,3) = -tmp9
      Gr(3,1) = -tmp8
      Gr(3,2) = tmp9
      Gr(3,3) = d
      
      DO I=1, 3
        DO J=1, 3
            R(I, J)=0D0
            DO K=1, 3
                R(I, J) = R(I, J) + RDelta(I, K) * R_0(K, J)
            END DO
        END DO
      END DO
      
      DO I=1, 3
        W(I) = 0D0
        DO J=1, 3
            W(I) = W(I) + Gr(I, J) * gP(J) + RDelta(I, J) * W_0(J)
        END DO
      END DO
      
      END
      

C        Generated by TAPENADE     (INRIA, Tropics team)
C  Tapenade 3.5 (r3618) - 22 Dec 2010 11:39
C
C  Differentiation of def_joint_ad_ass_res in forward (tangent) mode: (multi-directional mode)
C   variations   of useful results: f1 f2 m1 m2
C   with respect to varying inputs: gp1 gp2 xp1 xp2 g1 g2 x1 x2
C   RW status of diff variables: gp1:in gp2:in f1:out f2:out m1:out
C                m2:out xp1:in xp2:in g1:in g2:in x1:in x2:in
      SUBROUTINE DEF_JOINT_AD_ASS_RES_DV(x1, x1d, g1, g1d, xp1, xp1d, 
     +                                   gp1, gp1d, r1_0, w1_0, x2, x2d
     +                                   , g2, g2d, xp2, xp2d, gp2, gp2d
     +                                   , r2_0, w2_0, s1, d1, o1, o2, 
     +                                   f1, f1d, m1, m1d, f2, f2d, m2, 
     +                                   m2d, nbdirs)
      IMPLICIT INTEGER (N-N)
      PARAMETER (nbdirsmax=12)
C  Hint: nbdirsmax should be the maximum number of differentiation directions
      DOUBLE PRECISION x1(3), g1(3), xp1(3), gp1(3), r1_0(3, 3), w1_0(3)
      DOUBLE PRECISION x1d(nbdirsmax, 3), g1d(nbdirsmax, 3), xp1d(
     +                 nbdirsmax, 3), gp1d(nbdirsmax, 3)
      DOUBLE PRECISION x2(3), g2(3), xp2(3), gp2(3), r2_0(3, 3), w2_0(3)
      DOUBLE PRECISION x2d(nbdirsmax, 3), g2d(nbdirsmax, 3), xp2d(
     +                 nbdirsmax, 3), gp2d(nbdirsmax, 3)
      DOUBLE PRECISION s1(3, 3), d1(3, 3), o1(3), o2(3)
      DOUBLE PRECISION r1(3, 3), w1(3)
      DOUBLE PRECISION r1d(nbdirsmax, 3, 3), w1d(nbdirsmax, 3)
      DOUBLE PRECISION r2(3, 3), w2(3)
      DOUBLE PRECISION r2d(nbdirsmax, 3, 3), w2d(nbdirsmax, 3)
      DOUBLE PRECISION r1o1(3), r2o2(3)
      DOUBLE PRECISION r1o1d(nbdirsmax, 3), r2o2d(nbdirsmax, 3)
      DOUBLE PRECISION dxr1(3), dxpr1(3), dx(3), dxp(3)
      DOUBLE PRECISION dxr1d(nbdirsmax, 3), dxpr1d(nbdirsmax, 3), dxd(
     +                 nbdirsmax, 3), dxpd(nbdirsmax, 3)
      DOUBLE PRECISION w1xr1o1(3), w2xr2o2(3)
      DOUBLE PRECISION w1xr1o1d(nbdirsmax, 3), w2xr2o2d(nbdirsmax, 3)
      DOUBLE PRECISION f1r1(3), f1(3), m1(3), f2(3), m2(3)
      DOUBLE PRECISION f1r1d(nbdirsmax, 3), f1d(nbdirsmax, 3), m1d(
     +                 nbdirsmax, 3), f2d(nbdirsmax, 3), m2d(nbdirsmax, 
     +                 3)
      INTEGER i, j
      EXTERNAL UPDATE_ROT
      EXTERNAL UPDATE_ROT_DV
      INTEGER nd
      INTEGER nbdirs
      INTEGER ii1
      CALL UPDATE_ROT_DV(g1, g1d, gp1, gp1d, r1_0, w1_0, r1, r1d, w1, 
     +                   w1d, nbdirs)
      CALL UPDATE_ROT_DV(g2, g2d, gp2, gp2d, r2_0, w2_0, r2, r2d, w2, 
     +                   w2d, nbdirs)
      DO nd=1,nbdirs
        DO ii1=1,3
          r2o2d(nd, ii1) = 0.D0
        ENDDO
      ENDDO
      DO nd=1,nbdirs
        DO ii1=1,3
          r1o1d(nd, ii1) = 0.D0
        ENDDO
      ENDDO
      DO i=1,3
        DO nd=1,nbdirs
          r1o1d(nd, i) = 0.D0
          r2o2d(nd, i) = 0.D0
        ENDDO
        r1o1(i) = 0d0
        r2o2(i) = 0d0
        DO j=1,3
          DO nd=1,nbdirs
            r1o1d(nd, i) = r1o1d(nd, i) + o1(j)*r1d(nd, i, j)
            r2o2d(nd, i) = r2o2d(nd, i) + o2(j)*r2d(nd, i, j)
          ENDDO
          r1o1(i) = r1o1(i) + r1(i, j)*o1(j)
          r2o2(i) = r2o2(i) + r2(i, j)*o2(j)
        ENDDO
      ENDDO
      DO nd=1,nbdirs
        DO ii1=1,3
          w1xr1o1d(nd, ii1) = 0.D0
        ENDDO
        w1xr1o1d(nd, 1) = w1d(nd, 2)*r1o1(3) + w1(2)*r1o1d(nd, 3) - 
     +    r1o1d(nd, 2)*w1(3) - r1o1(2)*w1d(nd, 3)
        w1xr1o1d(nd, 2) = r1o1d(nd, 1)*w1(3) + r1o1(1)*w1d(nd, 3) - w1d(
     +    nd, 1)*r1o1(3) - w1(1)*r1o1d(nd, 3)
        w1xr1o1d(nd, 3) = w1d(nd, 1)*r1o1(2) + w1(1)*r1o1d(nd, 2) - 
     +    r1o1d(nd, 1)*w1(2) - r1o1(1)*w1d(nd, 2)
        DO ii1=1,3
          w2xr2o2d(nd, ii1) = 0.D0
        ENDDO
        w2xr2o2d(nd, 1) = w2d(nd, 2)*r2o2(3) + w2(2)*r2o2d(nd, 3) - 
     +    r2o2d(nd, 2)*w2(3) - r2o2(2)*w2d(nd, 3)
        w2xr2o2d(nd, 2) = r2o2d(nd, 1)*w2(3) + r2o2(1)*w2d(nd, 3) - w2d(
     +    nd, 1)*r2o2(3) - w2(1)*r2o2d(nd, 3)
        w2xr2o2d(nd, 3) = w2d(nd, 1)*r2o2(2) + w2(1)*r2o2d(nd, 2) - 
     +    r2o2d(nd, 1)*w2(2) - r2o2(1)*w2d(nd, 2)
      ENDDO
      w1xr1o1(1) = w1(2)*r1o1(3) - r1o1(2)*w1(3)
      w1xr1o1(2) = r1o1(1)*w1(3) - w1(1)*r1o1(3)
      w1xr1o1(3) = w1(1)*r1o1(2) - r1o1(1)*w1(2)
      w2xr2o2(1) = w2(2)*r2o2(3) - r2o2(2)*w2(3)
      w2xr2o2(2) = r2o2(1)*w2(3) - w2(1)*r2o2(3)
      w2xr2o2(3) = w2(1)*r2o2(2) - r2o2(1)*w2(2)
      DO nd=1,nbdirs
        DO ii1=1,3
          dxr1d(nd, ii1) = 0.D0
        ENDDO
      ENDDO
      DO nd=1,nbdirs
        DO ii1=1,3
          dxpr1d(nd, ii1) = 0.D0
        ENDDO
      ENDDO
      DO i=1,3
        DO nd=1,nbdirs
          dxr1d(nd, i) = x1d(nd, i) + r1o1d(nd, i) - x2d(nd, i) - r2o2d(
     +      nd, i)
          dxpr1d(nd, i) = xp1d(nd, i) + w1xr1o1d(nd, i) - xp2d(nd, i) - 
     +      w2xr2o2d(nd, i)
        ENDDO
        dxr1(i) = x1(i) + r1o1(i) - x2(i) - r2o2(i)
        dxpr1(i) = xp1(i) + w1xr1o1(i) - xp2(i) - w2xr2o2(i)
      ENDDO
      DO nd=1,nbdirs
        DO ii1=1,3
          dxd(nd, ii1) = 0.D0
        ENDDO
      ENDDO
      DO nd=1,nbdirs
        DO ii1=1,3
          dxpd(nd, ii1) = 0.D0
        ENDDO
      ENDDO
      DO i=1,3
        DO nd=1,nbdirs
          dxd(nd, i) = 0.D0
          dxpd(nd, i) = 0.D0
        ENDDO
        dx(i) = 0d0
        dxp(i) = 0d0
        DO j=1,3
          DO nd=1,nbdirs
            dxd(nd, i) = dxd(nd, i) + r1d(nd, j, i)*dxr1(j) + r1(j, i)*
     +        dxr1d(nd, j)
            dxpd(nd, i) = dxpd(nd, i) + r1d(nd, j, i)*dxpr1(j) + r1(j, i
     +        )*dxpr1d(nd, j)
          ENDDO
          dx(i) = dx(i) + r1(j, i)*dxr1(j)
          dxp(i) = dxp(i) + r1(j, i)*dxpr1(j)
        ENDDO
      ENDDO
      DO nd=1,nbdirs
        DO ii1=1,3
          f1r1d(nd, ii1) = 0.D0
        ENDDO
      ENDDO
      DO i=1,3
        DO nd=1,nbdirs
          f1r1d(nd, i) = 0.D0
        ENDDO
        f1r1(i) = 0d0
        DO j=1,3
          DO nd=1,nbdirs
            f1r1d(nd, i) = f1r1d(nd, i) + s1(i, j)*dxd(nd, j) + d1(i, j)
     +        *dxpd(nd, j)
          ENDDO
          f1r1(i) = f1r1(i) + s1(i, j)*dx(j) + d1(i, j)*dxp(j)
        ENDDO
      ENDDO
      DO nd=1,nbdirs
        DO ii1=1,3
          f1d(nd, ii1) = 0.D0
        ENDDO
      ENDDO
      DO nd=1,nbdirs
        DO ii1=1,3
          f2d(nd, ii1) = 0.D0
        ENDDO
      ENDDO
      DO i=1,3
        DO nd=1,nbdirs
          f1d(nd, i) = 0.D0
        ENDDO
        f1(i) = 0d0
        DO j=1,3
          DO nd=1,nbdirs
            f1d(nd, i) = f1d(nd, i) - r1d(nd, i, j)*f1r1(j) - r1(i, j)*
     +        f1r1d(nd, j)
          ENDDO
          f1(i) = f1(i) - r1(i, j)*f1r1(j)
        ENDDO
        DO nd=1,nbdirs
          f2d(nd, i) = -f1d(nd, i)
        ENDDO
        f2(i) = -f1(i)
      ENDDO
      DO nd=1,nbdirs
        DO ii1=1,3
          m1d(nd, ii1) = 0.D0
        ENDDO
        m1d(nd, 1) = r1o1d(nd, 2)*f1(3) + r1o1(2)*f1d(nd, 3) - f1d(nd, 2
     +    )*r1o1(3) - f1(2)*r1o1d(nd, 3)
        m1d(nd, 2) = f1d(nd, 1)*r1o1(3) + f1(1)*r1o1d(nd, 3) - r1o1d(nd
     +    , 1)*f1(3) - r1o1(1)*f1d(nd, 3)
        m1d(nd, 3) = r1o1d(nd, 1)*f1(2) + r1o1(1)*f1d(nd, 2) - f1d(nd, 1
     +    )*r1o1(2) - f1(1)*r1o1d(nd, 2)
        DO ii1=1,3
          m2d(nd, ii1) = 0.D0
        ENDDO
        m2d(nd, 1) = r2o2d(nd, 2)*f2(3) + r2o2(2)*f2d(nd, 3) - f2d(nd, 2
     +    )*r2o2(3) - f2(2)*r2o2d(nd, 3)
        m2d(nd, 2) = f2d(nd, 1)*r2o2(3) + f2(1)*r2o2d(nd, 3) - r2o2d(nd
     +    , 1)*f2(3) - r2o2(1)*f2d(nd, 3)
        m2d(nd, 3) = r2o2d(nd, 1)*f2(2) + r2o2(1)*f2d(nd, 2) - f2d(nd, 1
     +    )*r2o2(2) - f2(1)*r2o2d(nd, 2)
      ENDDO
      m1(1) = r1o1(2)*f1(3) - f1(2)*r1o1(3)
      m1(2) = f1(1)*r1o1(3) - r1o1(1)*f1(3)
      m1(3) = r1o1(1)*f1(2) - f1(1)*r1o1(2)
      m2(1) = r2o2(2)*f2(3) - f2(2)*r2o2(3)
      m2(2) = f2(1)*r2o2(3) - r2o2(1)*f2(3)
      m2(3) = r2o2(1)*f2(2) - f2(1)*r2o2(2)
      END

C        Generated by TAPENADE     (INRIA, Tropics team)
C  Tapenade 3.5 (r3618) - 22 Dec 2010 11:39
C
C  Differentiation of update_rot in forward (tangent) mode: (multi-directional mode)
C   variations   of useful results: r w
C   with respect to varying inputs: g gp
      SUBROUTINE UPDATE_ROT_DV(g, gd, gp, gpd, r_0, w_0, r, rd, w, wd, 
     +                         nbdirs)
      IMPLICIT INTEGER (N-N)
      PARAMETER (nbdirsmax=12)
C  Hint: nbdirsmax should be the maximum number of differentiation directions
      DOUBLE PRECISION g(3), gp(3), r_0(3, 3), w_0(3)
      DOUBLE PRECISION gd(nbdirsmax, 3), gpd(nbdirsmax, 3)
      DOUBLE PRECISION r(3, 3), w(3)
      DOUBLE PRECISION rd(nbdirsmax, 3, 3), wd(nbdirsmax, 3)
      DOUBLE PRECISION rdelta(3, 3), gr(3, 3), d
      DOUBLE PRECISION rdeltad(nbdirsmax, 3, 3), grd(nbdirsmax, 3, 3), 
     +                 dd(nbdirsmax)
      DOUBLE PRECISION tmp1, tmp2, tmp3, tmp4, tmp5, tmp6
      DOUBLE PRECISION tmp1d(nbdirsmax), tmp2d(nbdirsmax), tmp3d(
     +                 nbdirsmax), tmp4d(nbdirsmax), tmp5d(nbdirsmax), 
     +                 tmp6d(nbdirsmax)
      DOUBLE PRECISION tmp7, tmp8, tmp9
      DOUBLE PRECISION tmp7d(nbdirsmax), tmp8d(nbdirsmax), tmp9d(
     +                 nbdirsmax)
      INTEGER i, j, k
      INTEGER nd
      INTEGER nbdirs
      INTEGER ii2
      INTEGER ii1
      tmp1 = g(3)**2
      tmp2 = g(2)**2
      tmp3 = g(1)**2
      tmp4 = g(1)*g(2)*0.5d0
      tmp5 = g(2)*g(3)*0.5d0
      tmp6 = g(1)*g(3)*0.5d0
      d = 4d0/(tmp1+tmp2+tmp3+4d0)
      DO nd=1,nbdirs
        tmp1d(nd) = 2*g(3)*gd(nd, 3)
        tmp2d(nd) = 2*g(2)*gd(nd, 2)
        tmp3d(nd) = 2*g(1)*gd(nd, 1)
        tmp4d(nd) = 0.5d0*(gd(nd, 1)*g(2)+g(1)*gd(nd, 2))
        tmp5d(nd) = 0.5d0*(gd(nd, 2)*g(3)+g(2)*gd(nd, 3))
        tmp6d(nd) = 0.5d0*(gd(nd, 1)*g(3)+g(1)*gd(nd, 3))
        dd(nd) = -(4d0*(tmp1d(nd)+tmp2d(nd)+tmp3d(nd))/(tmp1+tmp2+tmp3+
     +    4d0)**2)
        DO ii1=1,3
          DO ii2=1,3
            rdeltad(nd, ii2, ii1) = 0.D0
          ENDDO
        ENDDO
        rdeltad(nd, 1, 1) = 0.5d0*((-tmp1d(nd)-tmp2d(nd))*d+(-tmp1-tmp2)
     +    *dd(nd))
        rdeltad(nd, 1, 2) = (tmp4d(nd)-gd(nd, 3))*d + (tmp4-g(3))*dd(nd)
        rdeltad(nd, 1, 3) = (tmp6d(nd)+gd(nd, 2))*d + (tmp6+g(2))*dd(nd)
        rdeltad(nd, 2, 1) = (gd(nd, 3)+tmp4d(nd))*d + (g(3)+tmp4)*dd(nd)
        rdeltad(nd, 2, 2) = 0.5d0*((-tmp1d(nd)-tmp3d(nd))*d+(-tmp1-tmp3)
     +    *dd(nd))
        rdeltad(nd, 2, 3) = (tmp5d(nd)-gd(nd, 1))*d + (tmp5-g(1))*dd(nd)
        rdeltad(nd, 3, 1) = (tmp6d(nd)-gd(nd, 2))*d + (tmp6-g(2))*dd(nd)
        rdeltad(nd, 3, 2) = (tmp5d(nd)+gd(nd, 1))*d + (tmp5+g(1))*dd(nd)
        rdeltad(nd, 3, 3) = 0.5d0*((-tmp2d(nd)-tmp3d(nd))*d+(-tmp2-tmp3)
     +    *dd(nd))
        tmp7d(nd) = 0.5d0*(gd(nd, 3)*d+g(3)*dd(nd))
        tmp8d(nd) = 0.5d0*(gd(nd, 2)*d+g(2)*dd(nd))
        tmp9d(nd) = 0.5d0*(gd(nd, 1)*d+g(1)*dd(nd))
        DO ii1=1,3
          DO ii2=1,3
            grd(nd, ii2, ii1) = 0.D0
          ENDDO
        ENDDO
        grd(nd, 1, 1) = dd(nd)
        grd(nd, 1, 2) = -tmp7d(nd)
        grd(nd, 1, 3) = tmp8d(nd)
        grd(nd, 2, 1) = tmp7d(nd)
        grd(nd, 2, 2) = dd(nd)
        grd(nd, 2, 3) = -tmp9d(nd)
        grd(nd, 3, 1) = -tmp8d(nd)
        grd(nd, 3, 2) = tmp9d(nd)
        grd(nd, 3, 3) = dd(nd)
      ENDDO
      rdelta(1, 1) = (-tmp1-tmp2)*d*0.5d0 + 1d0
      rdelta(1, 2) = (tmp4-g(3))*d
      rdelta(1, 3) = (tmp6+g(2))*d
      rdelta(2, 1) = (g(3)+tmp4)*d
      rdelta(2, 2) = (-tmp1-tmp3)*d*0.5d0 + 1d0
      rdelta(2, 3) = (tmp5-g(1))*d
      rdelta(3, 1) = (tmp6-g(2))*d
      rdelta(3, 2) = (tmp5+g(1))*d
      rdelta(3, 3) = (-tmp2-tmp3)*d*0.5d0 + 1d0
      tmp7 = 0.5d0*g(3)*d
      tmp8 = 0.5d0*g(2)*d
      tmp9 = 0.5d0*g(1)*d
      gr(1, 1) = d
      gr(1, 2) = -tmp7
      gr(1, 3) = tmp8
      gr(2, 1) = tmp7
      gr(2, 2) = d
      gr(2, 3) = -tmp9
      gr(3, 1) = -tmp8
      gr(3, 2) = tmp9
      gr(3, 3) = d
      DO nd=1,nbdirs
        DO ii1=1,3
          DO ii2=1,3
            rd(nd, ii2, ii1) = 0.D0
          ENDDO
        ENDDO
      ENDDO
      DO i=1,3
        DO j=1,3
          DO nd=1,nbdirs
            rd(nd, i, j) = 0.D0
          ENDDO
          r(i, j) = 0d0
          DO k=1,3
            DO nd=1,nbdirs
              rd(nd, i, j) = rd(nd, i, j) + r_0(k, j)*rdeltad(nd, i, k)
            ENDDO
            r(i, j) = r(i, j) + rdelta(i, k)*r_0(k, j)
          ENDDO
        ENDDO
      ENDDO
      DO nd=1,nbdirs
        DO ii1=1,3
          wd(nd, ii1) = 0.D0
        ENDDO
      ENDDO
      DO i=1,3
        DO nd=1,nbdirs
          wd(nd, i) = 0.D0
        ENDDO
        w(i) = 0d0
        DO j=1,3
          DO nd=1,nbdirs
            wd(nd, i) = wd(nd, i) + grd(nd, i, j)*gp(j) + gr(i, j)*gpd(
     +        nd, j) + w_0(j)*rdeltad(nd, i, j)
          ENDDO
          w(i) = w(i) + gr(i, j)*gp(j) + rdelta(i, j)*w_0(j)
        ENDDO
      ENDDO
      END
