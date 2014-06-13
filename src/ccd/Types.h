/*
 * Types.h
 *
 *  Created on: Aug 20, 2013
 *      Author: msuchard
 */

#ifndef CCD_TYPES_H_
#define CCD_TYPES_H_

#include <vector>
#include <map>

#if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
// C++11
    #include <memory>
    namespace bsccs { 
        using std::shared_ptr;
        using std::make_shared;
    }
#else
// C++98 (R build)
    #include "boost/smart_ptr.hpp"
    namespace bsccs {
        using boost::shared_ptr;
        using boost::make_shared;
    }
#endif

// #ifdef R_BUILD  // old alternative -DR_BUILD
// #endif

namespace bsccs {

#ifdef USE_DRUG_STRING
	typedef string DrugIdType; // TODO Strings do not get sorted in numerical order
#else
	typedef int DrugIdType;
#endif

// Output types

typedef std::pair<std::string,double> ExtraInformation;
typedef std::vector<ExtraInformation> ExtraInformationVector;

struct ProfileInformation {
	bool defined;
	double lower95Bound;
	double upper95Bound;
	int evaluations;

	ProfileInformation() : defined(false), lower95Bound(0.0), upper95Bound(0.0), evaluations(0) { }
	ProfileInformation(double lower, double upper) : defined(true), lower95Bound(lower),
			upper95Bound(upper), evaluations(0) { }
	ProfileInformation(double lower, double upper, int evals) : defined(true), lower95Bound(lower),
			upper95Bound(upper), evaluations(evals) { } 
};

typedef std::map<DrugIdType, ProfileInformation> ProfileInformationMap;
typedef std::vector<ProfileInformation> ProfileInformationList;


#ifdef DOUBLE_PRECISION
	typedef double real;
#else
	typedef float real;
#endif 

namespace priors {

enum PriorType {
	NONE = 0,
	LAPLACE,
	NORMAL
};

} // namespace priors

enum ConvergenceType {
	GRADIENT,
	LANGE,
	MITTAL,
	ZHANG_OLES
};

enum NoiseLevels {
	SILENT = 0,
	QUIET,
	NOISY
};

enum UpdateReturnFlags {
	SUCCESS = 0,
	FAIL,
	MAX_ITERATIONS,
	ILLCONDITIONED,
	MISSING_COVARIATES
};

typedef std::vector<DrugIdType> ProfileVector;

namespace Models {

enum ModelType {
	NONE = 0,
	NORMAL,
	POISSON,
	LOGISTIC,
	CONDITIONAL_LOGISTIC,
	SELF_CONTROLLED_MODEL,
	COX
};

static bool requiresStratumID(const ModelType modelType) {
	return (modelType == CONDITIONAL_LOGISTIC || modelType == SELF_CONTROLLED_MODEL);
}

static bool requiresCensoredData(const ModelType modelType) {
	return (modelType == COX);
}

static bool requiresOffset(const ModelType modelType) {
	return (modelType == SELF_CONTROLLED_MODEL);
}

} // namespace Models

// Hierarchical prior types

typedef std::map<int, int> HierarchicalParentMap;
typedef std::map<int, std::vector<int> > HierarchicalChildMap;

} // namespace bsccs

#endif /* CCD_TYPES_H_ */
