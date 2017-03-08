#ifndef CLASS_ELEMENT_H
#define CLASS_ELEMENT_H

class ELEMENT {
protected:
    int ID;
    
public:
    ELEMENT(int ID) : ID(ID) {}
    ~ELEMENT() = default;

    virtual void ComputeInternalU(int) {};
    virtual void ComputeBoundaryU(int) {};

    virtual void CreateInterfaces() {};

	virtual double IntegrationInternalPhi(int, int) { return 0; };
    virtual double IntegrationInternalDPhiDX(int, int) { return 0; };
    virtual double IntegrationInternalDPhiDY(int, int) { return 0; };

    virtual double IntegrationBoundaryNX(int, int, int) { return 0; };
    virtual double IntegrationBoundaryNY(int, int, int) { return 0; };

    virtual double test_against_phi(double f_at_gp[]) = 0;
    virtual double test_against_dphidx(double f_at_gp[]) = 0;
    virtual double test_against_dphidy(double f_at_gp[]) = 0;

    virtual double* invert_mass_matrix(double f_bf[]) = 0;
};

#endif