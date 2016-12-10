/**
 *    > Author:            UncP
 *    > Mail:         770778010@qq.com
 *    > Github:    https://www.github.com/UncP/Bunny
 *    > Created Time:  2016-12-05 14:50:32
**/

#ifndef _BSDF_HPP_
#define _BSDF_HPP_

#include <random>
#include <ctime>

#include "constant.hpp"
#include "vector.hpp"
#include "point.hpp"
#include "parameter.hpp"

namespace Swan {

static std::default_random_engine generator(time(0));
static std::uniform_real_distribution<double> distribution(0, 1);

static inline double CosTheta(const Vector &wi)
{
	return wi.z_;
}

static inline double AbsCosTheta(const Vector &wi)
{
	return std::abs(wi.z_);
}

static inline Vector CosineWeightedHemisphere()
{
	double u1 = distribution(generator);
	double u2 = distribution(generator);

	double theta = std::asin(std::sqrt(u1));
	double phi = 2 * PI * u2;

	double sini = std::sin(theta);
	double cosi = std::cos(theta);

	double xs = sini * std::cos(phi);
	double ys = sini * std::sin(phi);
	double zs = cosi;

	return Normalize(Vector(xs, ys, zs));
}

double FresnelDielectric(double cosi, double etai, double etat)
{
	if (cosi < 0) {
		std::swap(etai, etat);
		cosi = -cosi;
	}

	double sini = std::sqrt(std::max(0.0, 1 - cosi * cosi));
	double sint = etai / etat * sini;

	if (sint >= 1) return 1;

	double cost = std::sqrt(std::max(0.0, 1 - sint * sint));

	double term1 = etai * cost;
	double term2 = etat * cosi;
	double term3 = etai * cosi;
	double term4 = etat * cost;

	double parl = (term2 - term1) / (term2 + term1);
	double perp = (term3 - term4) / (term3 + term4);
	return (parl * parl + perp * perp) * 0.5;
}

class BSDF
{
	public:
		BSDF(const Spectrum &r):r_(r) { }

		virtual Spectrum F(const Vector &wo, const Vector &wi) const = 0;

		virtual Spectrum SampleF(const Vector &wo, Vector &wi, double &pdf) const = 0;

		virtual bool IsDelta() const { return false; }

		virtual ~BSDF() { }

	protected:
		const Spectrum r_;
};

class DiffuseBSDF : public BSDF
{
	public:
		DiffuseBSDF(const Spectrum &r):BSDF(r) { }

		virtual Spectrum F(const Vector &wo, const Vector &wi) const override {
			return r_ * INV_PI;
		}

		virtual Spectrum SampleF(const Vector &wo, Vector &wi, double &pdf) const override {
			wi  = CosineWeightedHemisphere();
			pdf = CosTheta(wi) * INV_PI;
			return F(wo, wi);
		}
};

class ReflectBSDF : public BSDF
{
	public:
		ReflectBSDF(const Spectrum &r):BSDF(r) { }

		Spectrum F(const Vector &wo, const Vector &wi) const override {
			return Spectrum();
		}

		Spectrum SampleF(const Vector &wo, Vector &wi, double &pdf) const override {
			wi = Vector(-wo.x_, -wo.y_, wo.z_);
			pdf = 1.0;
			return r_ * (1.0 / CosTheta(wi));
		}

		bool IsDelta() const override { return true; }
};

class RefractBSDF : public BSDF
{
	public:
		RefractBSDF(const Spectrum &r, double etai, double etat)
		:BSDF(r), etai_(etai), etat_(etat) { }

		virtual Spectrum F(const Vector &wo, const Vector &wi) const override {
			return Spectrum();
		}

		virtual Spectrum SampleF(const Vector &wo, Vector &wi, double &pdf) const override {
			bool entering = CosTheta(wo) < 0;
			double etai = entering ? etai_ : etat_;
			double etat = entering ? etat_ : etai_;

			double eta = etai / etat;

			double cosi = AbsCosTheta(wo);

			double sini = std::max(0.0, 1 - cosi * cosi);
			double sint = eta * eta * sini;

			double re = 1, cost;
			if (sint < 1) {
				cost = std::sqrt(1 - sint);

				double term1 = etai * cost;
				double term2 = etat * cosi;
				double term3 = etai * cosi;
				double term4 = etat * cost;

				double parl = (term2 - term1) / (term2 + term1);
				double perp = (term3 - term4) / (term3 + term4);
				re = (parl * parl + perp * perp) * 0.5;

				wi = Normalize(wo * eta + Vector(0, 0, 1) * ((eta * cosi - cost) * (entering ? -1 : 1)));
				pdf = 1 - re;
				return r_ * (1 - re) / AbsCosTheta(wi);
			} else {
				wi = Vector(-wo.x_, -wo.y_, wo.z_);
				pdf = re;
				return r_ * re / AbsCosTheta(wi);
			}
		}

		bool IsDelta() const override { return true; }

	private:
		double etai_;
		double etat_;
};

BSDF* NewDiffuseBSDF(Parameter &param)
{
	Spectrum spectrum = param.FindVector();
	return new DiffuseBSDF(spectrum);
}

BSDF* NewReflectBSDF(Parameter &param)
{
	Spectrum spectrum = param.FindVector();
	return new ReflectBSDF(spectrum);
}

BSDF* NewRefractBSDF(Parameter &param)
{
	Spectrum spectrum = param.FindVector();
	double etai = param.FindDouble();
	double etat = param.FindDouble();
	return new RefractBSDF(spectrum, etai, etat);
}

} // namespace Swan

#endif /* _BSDF_HPP_ */