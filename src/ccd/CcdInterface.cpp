/*
 * ccd.cpp
 *
 *  Created on: July, 2010
 *      Author: msuchard
 */
 

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cstdlib>
#include <cstring>

#include <iostream>
#include <time.h>

#ifndef _MSC_VER
	#include <sys/time.h>
#endif

#include <math.h>

// #include "Types.h"
#include "CcdInterface.h"
#include "CyclicCoordinateDescent.h"
#include "ModelData.h"
// #include "io/InputReader.h"
// #include "io/HierarchyReader.h"
// #include "io/CLRInputReader.h"
// #include "io/RTestInputReader.h"
// #include "io/CCTestInputReader.h"
// #include "io/CoxInputReader.h"
// #include "io/NewCLRInputReader.h"
// #include "io/NewSCCSInputReader.h"
// #include "io/NewCoxInputReader.h"
// #include "io/NewGenericInputReader.h"
// #include "io/BBRInputReader.h"
// #include "io/OutputWriter.h"
#include "drivers/CrossValidationSelector.h"
#include "drivers/GridSearchCrossValidationDriver.h"
#include "drivers/HierarchyGridSearchCrossValidationDriver.h"
#include "drivers/AutoSearchCrossValidationDriver.h"
#include "drivers/HierarchyAutoSearchCrossValidationDriver.h"
#include "drivers/BootstrapSelector.h"
#include "drivers/ProportionSelector.h"
#include "drivers/BootstrapDriver.h"
#include "engine/ModelSpecifics.h"

// #include "tclap/CmdLine.h"
#include "utils/RZeroIn.h"

//#include <R.h>

#ifdef CUDA
	#include "GPUCyclicCoordinateDescent.h"
//	#include "BetterGPU.h"
#endif


#define NEW

namespace bsccs {

// using namespace TCLAP;
using namespace std;

//Sushil:Implementing gettimeofday functionality for windows.
#ifdef _MSC_VER
	#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
		#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
	#else
		#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
	#endif
	struct timezone
	{
		int  tz_minuteswest; /* minutes W of Greenwich */
		int  tz_dsttime;     /* type of dst correction */
	};

	// Definition of a gettimeofday function

	int gettimeofday(struct timeval *tv, struct timezone *tz)
	{
		// Define a structure to receive the current Windows filetime
		FILETIME ft;

		// Initialize the present time to 0 and the timezone to UTC
		unsigned __int64 tmpres = 0;
		static int tzflag = 0;

		if (NULL != tv)
		{
			GetSystemTimeAsFileTime(&ft);

			// The GetSystemTimeAsFileTime returns the number of 100 nanosecond
			// intervals since Jan 1, 1601 in a structure. Copy the high bits to
			// the 64 bit tmpres, shift it left by 32 then or in the low 32 bits.
			tmpres |= ft.dwHighDateTime;
			tmpres <<= 32;
			tmpres |= ft.dwLowDateTime;

			// Convert to microseconds by dividing by 10
			tmpres /= 10;

			// The Unix epoch starts on Jan 1 1970.  Need to subtract the difference
			// in seconds from Jan 1 1601.
			tmpres -= DELTA_EPOCH_IN_MICROSECS;

			// Finally change microseconds to seconds and place in the seconds value.
			// The modulus picks up the microseconds.
			tv->tv_sec = (long)(tmpres / 1000000UL);
			tv->tv_usec = (long)(tmpres % 1000000UL);
		}

		if (NULL != tz)
		{
			if (!tzflag)
			{
				_tzset();
				tzflag++;
			}

			// Adjust for the timezone west of Greenwich
			tz->tz_minuteswest = _timezone / 60;
			tz->tz_dsttime = _daylight;
		}

		return 0;
	}
#endif

CcdInterface::CcdInterface(void) {
    setDefaultArguments();
}

CcdInterface::~CcdInterface(void) {
    // Do nothing
}

double CcdInterface::calculateSeconds(const timeval &time1, const timeval &time2) {
	return time2.tv_sec - time1.tv_sec +
			(double)(time2.tv_usec - time1.tv_usec) / 1000000.0;
}

void CcdInterface::setDefaultArguments(void) {
	arguments.useGPU = false;
	arguments.maxIterations = 1000;
	arguments.inFileName = "default_in";
	arguments.outFileName = "default_out";
	arguments.outDirectoryName = "";
	arguments.hyperPriorSet = false;
	arguments.hyperprior = 1.0;
	arguments.tolerance = 1E-6; //5E-4;
	arguments.seed = 123;
	arguments.doCrossValidation = false;
	arguments.useAutoSearchCV = false;
	arguments.lowerLimit = 0.01;
	arguments.upperLimit = 20.0;
	arguments.fold = 10;
	arguments.gridSteps = 10;
	arguments.cvFileName = "cv.txt";
	arguments.useHierarchy = false;
	arguments.doBootstrap = false;
	arguments.replicates = 100;
	arguments.reportRawEstimates = false;
	arguments.modelName = "sccs";
	arguments.fileFormat = "sccs";
	//arguments.outputFormat = "estimates";
	arguments.computeMLE = false;
	arguments.fitMLEAtMode = false;
	arguments.reportASE = false;
	arguments.useNormalPrior = false;
	arguments.convergenceType = GRADIENT;
	arguments.convergenceTypeString = "gradient";
	arguments.doPartial = false;
	arguments.noiseLevel = NOISY;
}


// void CcdInterface::parseCommandLine(std::vector<std::string>& args) {
// 
// 	setDefaultArguments();
// 
// 	try {
// 		CmdLine cmd("Cyclic coordinate descent algorithm for self-controlled case studies", ' ', "0.1");
// 		ValueArg<int> gpuArg("g","GPU","Use GPU device", arguments.useGPU, -1, "device #");
// //		SwitchArg betterGPUArg("1","better", "Use better GPU implementation", false);
// 		ValueArg<int> maxIterationsArg("", "maxIterations", "Maximum iterations", false, arguments.maxIterations, "int");
// 		UnlabeledValueArg<string> inFileArg("inFileName","Input file name", true, arguments.inFileName, "inFileName");
// 		UnlabeledValueArg<string> outFileArg("outFileName","Output file name", true, arguments.outFileName, "outFileName");
// 		ValueArg<string> outDirectoryNameArg("", "outDirectoryName", "Output directory name", false, arguments.outDirectoryName, "outDirectoryName");
// 
// 
// 		// Prior arguments
// 		ValueArg<double> hyperPriorArg("v", "variance", "Hyperprior variance", false, arguments.hyperprior, "real");
// 		SwitchArg normalPriorArg("n", "normalPrior", "Use normal prior, default is laplace", arguments.useNormalPrior);
// 		SwitchArg computeMLEArg("", "MLE", "Compute maximum likelihood estimates only", arguments.computeMLE);
// 		SwitchArg computeMLEAtModeArg("", "MLEAtMode", "Compute maximum likelihood estimates at posterior mode", arguments.fitMLEAtMode);
// 		SwitchArg reportASEArg("","ASE", "Compute asymptotic standard errors at posterior mode", arguments.reportASE);
// 
// 		//Hierarchy arguments
// 		SwitchArg useHierarchyArg("", "hier", "Use hierarchy in analysis", arguments.useHierarchy);
// 		ValueArg<string> hierarchyFileArg("a", "hierarchyFile", "Hierarchy file name", false, "noFileName", "hierarchyFile");
// 		ValueArg<double> classHierarchyVarianceArg("d","classHierarchyVariance","Variance for drug class hierarchy", false, 10, "Variance at the class level of the hierarchy");
// 
// 
// 		// Convergence criterion arguments
// 		ValueArg<double> toleranceArg("t", "tolerance", "Convergence criterion tolerance", false, arguments.tolerance, "real");
// //		SwitchArg zhangOlesConvergenceArg("z", "zhangOles", "Use Zhange-Oles convergence criterion, default is true", true);
// 		std::vector<std::string> allowedConvergence;
// 		allowedConvergence.push_back("gradient");
// 		allowedConvergence.push_back("ZhangOles");
// 		allowedConvergence.push_back("Lange");
// 		allowedConvergence.push_back("Mittal");
// 		ValuesConstraint<std::string> allowedConvergenceValues(allowedConvergence);
// 		ValueArg<string> convergenceArg("", "convergence", "Convergence criterion", false, arguments.convergenceTypeString, &allowedConvergenceValues);
// 
// 		ValueArg<long> seedArg("s", "seed", "Random number generator seed", false, arguments.seed, "long");
// 
// 		// Cross-validation arguments
// 		SwitchArg doCVArg("c", "cv", "Perform cross-validation selection of hyperprior variance", arguments.doCrossValidation);
// 		SwitchArg useAutoSearchCVArg("", "auto", "Use an auto-search when performing cross-validation", arguments.useAutoSearchCV);
// 		ValueArg<double> lowerCVArg("l", "lower", "Lower limit for cross-validation search", false, arguments.lowerLimit, "real");
// 		ValueArg<double> upperCVArg("u", "upper", "Upper limit for cross-validation search", false, arguments.upperLimit, "real");
// 		ValueArg<int> foldCVArg("f", "fold", "Fold level for cross-validation", false, arguments.fold, "int");
// 		ValueArg<int> gridCVArg("", "gridSize", "Uniform grid size for cross-validation search", false, arguments.gridSteps, "int");
// 		ValueArg<int> foldToComputeCVArg("", "computeFold", "Number of fold to iterate, default is 'fold' value", false, 10, "int");
// 		ValueArg<string> outFile2Arg("", "cvFileName", "Cross-validation output file name", false, arguments.cvFileName, "cvFileName");
// 
// 		// Bootstrap arguments
// 		SwitchArg doBootstrapArg("b", "bs", "Perform bootstrap estimation", arguments.doBootstrap);
// //		ValueArg<string> bsOutFileArg("", "bsFileName", "Bootstrap output file name", false, "bs.txt", "bsFileName");
// 		ValueArg<int> replicatesArg("r", "replicates", "Number of bootstrap replicates", false, arguments.replicates, "int");
// 		SwitchArg reportRawEstimatesArg("","raw", "Report the raw bootstrap estimates", arguments.reportRawEstimates);
// 		ValueArg<int> partialArg("", "partial", "Number of rows to use in partial estimation", false, -1, "int");
// 
// 		// Model arguments
// //		SwitchArg doLogisticRegressionArg("", "logistic", "Use ordinary logistic regression", arguments.doLogisticRegression);
// 		std::vector<std::string> allowedModels;
// 		allowedModels.push_back("sccs");
// 		allowedModels.push_back("clr");
// 		allowedModels.push_back("lr");
// 		allowedModels.push_back("ls");
// 		allowedModels.push_back("pr");
// 		allowedModels.push_back("cox");
// 		ValuesConstraint<std::string> allowedModelValues(allowedModels);
// 		ValueArg<string> modelArg("", "model", "Model specification", false, arguments.modelName, &allowedModelValues);
// 
// 		// Format arguments
// 		std::vector<std::string> allowedFormats;
// 		allowedFormats.push_back("sccs");
// 		allowedFormats.push_back("clr");
// 		allowedFormats.push_back("csv");
// 		allowedFormats.push_back("cc");
// 		allowedFormats.push_back("cox-csv");
// 		allowedFormats.push_back("new-cox");
// 		allowedFormats.push_back("bbr");
// 		allowedFormats.push_back("generic");
// 		ValuesConstraint<std::string> allowedFormatValues(allowedFormats);
// 		ValueArg<string> formatArg("", "format", "Format of data file", false, arguments.fileFormat, &allowedFormatValues);
// 
// 		// Output format arguments
// 		std::vector<std::string> allowedOutputFormats;
// 		allowedOutputFormats.push_back("estimates");
// 		allowedOutputFormats.push_back("prediction");
// 		allowedOutputFormats.push_back("diagnostics");
// 		ValuesConstraint<std::string> allowedOutputFormatValues(allowedOutputFormats);
// //		ValueArg<string> outputFormatArg("", "outputFormat", "Format of the output file", false, arguments.outputFormat, &allowedOutputFormatValues);
// 		MultiArg<std::string> outputFormatArg("", "output", "Format of the output file", false, &allowedOutputFormatValues);
// 
// 		// Control screen output volume
// 		SwitchArg quietArg("q", "quiet", "Limit writing to standard out", arguments.noiseLevel <= QUIET);
// 
// 		MultiArg<int> profileCIArg("","profileCI", "Report confidence interval for covariate", false, "integer");
// 		MultiArg<int> flatPriorArg("","flat", "Place no prior on covariate", false, "integer");
// 
// 
// 		cmd.add(gpuArg);
// //		cmd.add(betterGPUArg);
// 		cmd.add(toleranceArg);
// 		cmd.add(maxIterationsArg);
// 		cmd.add(hyperPriorArg);
// 		cmd.add(normalPriorArg);
// 		cmd.add(computeMLEArg);
// 		cmd.add(computeMLEAtModeArg);
// 		cmd.add(reportASEArg);
// //		cmd.add(zhangOlesConvergenceArg);
// 		cmd.add(convergenceArg);
// 		cmd.add(seedArg);
// 		cmd.add(modelArg);
// 		cmd.add(formatArg);
// 		cmd.add(outputFormatArg);
// 		cmd.add(profileCIArg);
// 		cmd.add(flatPriorArg);
// 
// 		//Hierarchy arguments
// 		cmd.add(useHierarchyArg);
// 		cmd.add(hierarchyFileArg);
// 		cmd.add(classHierarchyVarianceArg);
// 
// 		cmd.add(doCVArg);
// 		cmd.add(useAutoSearchCVArg);
// 		cmd.add(lowerCVArg);
// 		cmd.add(upperCVArg);
// 		cmd.add(foldCVArg);
// 		cmd.add(gridCVArg);
// 		cmd.add(foldToComputeCVArg);
// 		cmd.add(outFile2Arg);
// 		cmd.add(outDirectoryNameArg);
// 
// 		cmd.add(doBootstrapArg);
// //		cmd.add(bsOutFileArg);
// 		cmd.add(replicatesArg);
// 		cmd.add(partialArg);
// 		cmd.add(reportRawEstimatesArg);
// //		cmd.add(doLogisticRegressionArg);
// 
// 		cmd.add(quietArg);
// 
// 		cmd.add(inFileArg);
// 		cmd.add(outFileArg);
// 		cmd.parse(args);
// 
// 		if (gpuArg.getValue() > -1) {
// 			arguments.useGPU = true;
// 			arguments.deviceNumber = gpuArg.getValue();
// 		} else {
// 			arguments.useGPU = false;
// 		}
// //		arguments.useBetterGPU = betterGPUArg.isSet();
// 
// 		arguments.inFileName = inFileArg.getValue();
// 		arguments.outFileName = outFileArg.getValue();
// 		arguments.outDirectoryName = outDirectoryNameArg.getValue();
// 		arguments.tolerance = toleranceArg.getValue();
// 		arguments.maxIterations = maxIterationsArg.getValue();
// 		arguments.hyperprior = hyperPriorArg.getValue();
// 		arguments.useNormalPrior = normalPriorArg.getValue();
// 		arguments.computeMLE = computeMLEArg.getValue();
// 		arguments.fitMLEAtMode = computeMLEAtModeArg.getValue();
// 		arguments.reportASE = reportASEArg.getValue();
// 		arguments.seed = seedArg.getValue();
// 
// 		//Hierarchy arguments
// 		arguments.useHierarchy = useHierarchyArg.isSet();
// 		arguments.hierarchyFileName = hierarchyFileArg.getValue(); // Hierarchy argument
// 		arguments.classHierarchyVariance = classHierarchyVarianceArg.getValue(); //Hierarchy argument
// 
// 		arguments.modelName = modelArg.getValue();
// 		arguments.fileFormat = formatArg.getValue();
// 		arguments.outputFormat = outputFormatArg.getValue();
// 		if (arguments.outputFormat.size() == 0) {
// 			arguments.outputFormat.push_back("estimates");
// 		}
// 		arguments.profileCI = profileCIArg.getValue();
// 		arguments.flatPrior = flatPriorArg.getValue();
// 
// 		arguments.convergenceTypeString = convergenceArg.getValue();
// 
// 		if (hyperPriorArg.isSet()) {
// 			arguments.hyperPriorSet = true;
// 		} else {
// 			arguments.hyperPriorSet = false;
// 		}
// 
// //		if (zhangOlesConvergenceArg.isSet()) {
// //			arguments.convergenceType = ZHANG_OLES;
// //		} else {
// //			arguments.convergenceType = LANGE;
// //		}
// 		if (arguments.convergenceTypeString == "ZhangOles") {
// 			arguments.convergenceType = ZHANG_OLES;
// 		} else if (arguments.convergenceTypeString == "Lange") {
// 			arguments.convergenceType = LANGE;
// 		} else if (arguments.convergenceTypeString == "Mittal") {
// 			arguments.convergenceType = MITTAL;
// 		} else if (arguments.convergenceTypeString == "gradient") {
// 			arguments.convergenceType = GRADIENT;
// 		} else {
// 			cerr << "Unknown convergence type: " << convergenceArg.getValue() << " " << arguments.convergenceTypeString << endl;
// 			exit(-1);
// 		}
// 
// 		// Cross-validation
// 		arguments.doCrossValidation = doCVArg.isSet();
// 		if (arguments.doCrossValidation) {
// 			arguments.useAutoSearchCV = useAutoSearchCVArg.isSet();
// 			arguments.lowerLimit = lowerCVArg.getValue();
// 			arguments.upperLimit = upperCVArg.getValue();
// 			arguments.fold = foldCVArg.getValue();
// 			arguments.gridSteps = gridCVArg.getValue();
// 			if(foldToComputeCVArg.isSet()) {
// 				arguments.foldToCompute = foldToComputeCVArg.getValue();
// 			} else {
// 				arguments.foldToCompute = arguments.fold;
// 			}
// 			arguments.cvFileName = outFile2Arg.getValue();
// 			arguments.doFitAtOptimal = true;
// 		}
// 
// 		// Bootstrap
// 		arguments.doBootstrap = doBootstrapArg.isSet();
// 		if (arguments.doBootstrap) {
// //			arguments.bsFileName = bsOutFileArg.getValue();
// 			arguments.replicates = replicatesArg.getValue();
// 			if (reportRawEstimatesArg.isSet()) {
// 				arguments.reportRawEstimates = true;
// 			} else {
// 				arguments.reportRawEstimates = false;
// 			}
// 		}
// 
// 		if (partialArg.getValue() != -1) {
// 			arguments.doPartial = true;
// 			arguments.replicates = partialArg.getValue();
// 		}
// 
// 		if (quietArg.getValue()) {
// 			arguments.noiseLevel = QUIET;
// 		}
// 
// //		arguments.doLogisticRegression = doLogisticRegressionArg.isSet();
// 	} catch (ArgException &e) {
// 		cerr << "Error: " << e.error() << " for argument " << e.argId() << endl;
// 		exit(-1);
// 	}
// }

double CcdInterface::initializeModel(
		ModelData** modelData,
		CyclicCoordinateDescent** ccd,
		AbstractModelSpecifics** model) {
    return initializeModelImpl(modelData, ccd, model);		
}

// double CmdLineCcdInterface::initializeModelImpl(
// 		ModelData** modelData,
// 		CyclicCoordinateDescent** ccd,
// 		AbstractModelSpecifics** model) {
// 	
// 	// TODO Break up function; too long
// 
// 	cout << "Running CCD (" <<
// #ifdef DOUBLE_PRECISION
// 	"double"
// #else
// 	"single"
// #endif
// 	"-precision) ..." << endl;
// 
// 	struct timeval time1, time2;
// 	gettimeofday(&time1, NULL);
// 
// 	// Parse type of model
// 	//using namespace bsccs::Models;
// 	bsccs::Models::ModelType modelType;
// 	if (arguments.modelName == "sccs") {
// 		modelType = bsccs::Models::SELF_CONTROLLED_MODEL;
// 	} else if (arguments.modelName == "clr") {
// 		modelType = bsccs::Models::CONDITIONAL_LOGISTIC;
// 	} else if (arguments.modelName == "lr") {
// 		modelType = bsccs::Models::LOGISTIC;
// 	} else if (arguments.modelName == "ls") {
// 		modelType = bsccs::Models::NORMAL;
// 	} else if (arguments.modelName == "pr") {
// 		modelType = bsccs::Models::POISSON;
// 	} else if (arguments.modelName == "cox") {
// 		modelType = bsccs::Models::COX;
// 	} else {
// 		cerr << "Invalid model type." << endl;
// 		exit(-1);
// 	}
// 
// 	InputReader* reader;
// 	if (arguments.fileFormat == "sccs") {
// 		reader = new SCCSInputReader();
// 	} else if (arguments.fileFormat == "clr") {
// 		reader = new NewCLRInputReader();
// 	} else if (arguments.fileFormat == "csv") {
// 		reader = new RTestInputReader();
// 	} else if (arguments.fileFormat == "cc") {
// 		reader = new CCTestInputReader();
// 	} else if (arguments.fileFormat == "cox-csv") {
// 		reader = new CoxInputReader();
// 	} else if (arguments.fileFormat == "bbr") {
// 		reader = new BBRInputReader<NoImputation>();
// 	} else if (arguments.fileFormat == "generic") {
// 		reader = new NewGenericInputReader(modelType);
// 	} else if (arguments.fileFormat == "new-cox") {
// 		reader = new NewCoxInputReader();
// 	} else {
// 		cerr << "Invalid file format." << endl;
// 		exit(-1);
// 	}
// 
// 	reader->readFile(arguments.inFileName.c_str()); // TODO Check for error
// 	// delete reader;
// 	*modelData = reader->getModelData();
// 
// 	switch (modelType) {
// 		case bsccs::Models::SELF_CONTROLLED_MODEL :
// 			*model = new ModelSpecifics<SelfControlledCaseSeries<real>,real>(**modelData);
// 			break;
// 		case bsccs::Models::CONDITIONAL_LOGISTIC :
// 			*model = new ModelSpecifics<ConditionalLogisticRegression<real>,real>(**modelData);
// 			break;
// 		case bsccs::Models::LOGISTIC :
// 			*model = new ModelSpecifics<LogisticRegression<real>,real>(**modelData);
// 			break;
// 		case bsccs::Models::NORMAL :
// 			*model = new ModelSpecifics<LeastSquares<real>,real>(**modelData);
// 			break;
// 		case bsccs::Models::POISSON :
// 			*model = new ModelSpecifics<PoissonRegression<real>,real>(**modelData);
// 			break;
// 		case bsccs::Models::COX :
// 			*model = new ModelSpecifics<CoxProportionalHazards<real>,real>(**modelData);
// 			break;
// 		default:
// 			cerr << "Invalid model type." << endl;
// 			exit(-1);
// 	}
// 
// #ifdef CUDA
// 	if (arguments.useGPU) {
// 		*ccd = new GPUCyclicCoordinateDescent(arguments.deviceNumber, *reader, **model);
// 	} else {
// #endif
// 
// 	// Hierarchy management
// 	HierarchyReader* hierarchyData;
// 	if (arguments.useHierarchy) {
// 		hierarchyData = new HierarchyReader(arguments.hierarchyFileName.c_str(), *modelData);
// 	}
// 
// 
// 	using namespace bsccs::priors;
// 	PriorPtr singlePrior;
// 	if (arguments.useNormalPrior) {
// 		singlePrior = std::make_shared<NormalPrior>();
// 	} else if (arguments.computeMLE) {
// 		if (arguments.fitMLEAtMode) {
// 			cerr << "Unable to compute MLE at posterior mode, if mode is not first explored." << endl;
// 			exit(-1);
// 		}
// 		singlePrior = std::make_shared<NoPrior>();
// 	} else {
// 		singlePrior = std::make_shared<LaplacePrior>();
// 	}
// 	singlePrior->setVariance(arguments.hyperprior);
// 
// 	JointPriorPtr prior;
// 	if (arguments.flatPrior.size() == 0) {
// 		prior = std::make_shared<FullyExchangeableJointPrior>(singlePrior);
// 	} else {
// 		const int length =  (*modelData)->getNumberOfColumns();
// 		std::shared_ptr<MixtureJointPrior> mixturePrior = std::make_shared<MixtureJointPrior>(
// 						singlePrior, length
// 				);
// 
// 		PriorPtr noPrior = std::make_shared<NoPrior>();
// 		for (ProfileVector::const_iterator it = arguments.flatPrior.begin();
// 				it != arguments.flatPrior.end(); ++it) {
// 			int index = (*modelData)->getColumnIndexByName(*it);
// 			if (index == -1) {
// 				cerr << "Variable " << *it << " not found." << endl;
// 			} else {
// 				mixturePrior->changePrior(noPrior, index);
// 			}
// 		}
// 		prior = mixturePrior;
// 	}
// 
// 	//Hierarchy prior
// 	if (arguments.useHierarchy) {
// 		std::shared_ptr<HierarchicalJointPrior> hierarchicalPrior = std::make_shared<HierarchicalJointPrior>(singlePrior, 2); //Depth of hierarchy fixed at 2 right now
// 		PriorPtr classPrior = std::make_shared<NormalPrior>();
// 		hierarchicalPrior->changePrior(classPrior,1);
// 		hierarchicalPrior->setHierarchy(hierarchyData);
// 		hierarchicalPrior->setVariance(0,arguments.hyperprior);
// 		hierarchicalPrior->setVariance(1,arguments.classHierarchyVariance);
// 		prior = hierarchicalPrior;
// 	}
// 
// 	*ccd = new CyclicCoordinateDescent(*modelData /* TODO Change to ref */, **model, prior);
// 
// #ifdef CUDA
// 	}
// #endif
// 
// 	(*ccd)->setNoiseLevel(arguments.noiseLevel);
// 
// 	gettimeofday(&time2, NULL);
// 	double sec1 = calculateSeconds(time1, time2);
// 
// 	cout << "Everything loaded and ready to run ..." << endl;
// 	
// 	return sec1;
// }

std::string CcdInterface::getPathAndFileName(const CCDArguments& arguments, std::string stem) {
	string fileName;
	if (arguments.outputFormat.size() == 1) {
		fileName = arguments.outDirectoryName + arguments.outFileName;
	} else {
		fileName = arguments.outDirectoryName + stem + arguments.outFileName;
	}
	return fileName;
}

double CcdInterface::predictModel(CyclicCoordinateDescent *ccd, ModelData *modelData) {
    return predictModelImpl(ccd, modelData);
}

// double CmdLineCcdInterface::predictModelImpl(CyclicCoordinateDescent *ccd, ModelData *modelData) {
// 
// 	struct timeval time1, time2;
// 	gettimeofday(&time1, NULL);
// 
// 	bsccs::PredictionOutputWriter predictor(*ccd, *modelData);
// 
// 	string fileName = getPathAndFileName(arguments, "pred_");
// 
// 	predictor.writeFile(fileName.c_str());
// 
// 	gettimeofday(&time2, NULL);
// 	return calculateSeconds(time1, time2);
// }

struct OptimizationProfile {

	CyclicCoordinateDescent& ccd;

	OptimizationProfile(CyclicCoordinateDescent& _ccd, int _index, double _max,
			double _threshold = 1.92) :
			ccd(_ccd), index(_index), max(_max), threshold(_threshold), nEvals(0) {
	}

	int getEvaluations() {
		return nEvals;
	}

	double objective(double x) {
		++nEvals;
		ccd.setBeta(index, x);
		return ccd.getLogLikelihood() + threshold - max;
	}

	double getMaximum() {
		return threshold;
	}

	int index;
	double max;
	double threshold;
	int nEvals;
};

double CcdInterface::profileModel(CyclicCoordinateDescent *ccd, ModelData *modelData,
		ProfileInformationMap& profileMap) {

	struct timeval time1, time2;
	gettimeofday(&time1, NULL);

	// Attempt profile CIs
	for (std::vector<DrugIdType>::iterator it = arguments.profileCI.begin();
			it != arguments.profileCI.end(); ++it) {
		int index = modelData->getColumnIndexByName(*it);
		if (index == -1) {
			cerr << "Variable " << *it << " not found." << endl;
		} else {
			// TODO : Minor bug, order of column evaluation yields different estimates
			double mode = ccd->getLogLikelihood();

			// TODO Check prior on covariate

			// Bound edge
			OptimizationProfile upEval(*ccd, index, mode);
			RZeroIn<OptimizationProfile> zeroIn(upEval);

			double x0 = ccd->getBeta(index);
			double obj0 = upEval.getMaximum();

			RZeroIn<OptimizationProfile>::Coordinate upperBracket =
					zeroIn.bracketSignChange(x0, obj0, 1.0);
			double upperPt = zeroIn.getRoot(x0, upperBracket.first, obj0, upperBracket.second);

			RZeroIn<OptimizationProfile>::Coordinate lowerBracket =
					zeroIn.bracketSignChange(x0, obj0, -1.0);
			double lowerPt = zeroIn.getRoot(x0, lowerBracket.first, obj0, lowerBracket.second);

			cout << "Profile: " << modelData->getColumn(index).getLabel() << " (" << lowerPt << ", "
					<< upperPt << ")  in " << upEval.getEvaluations() << endl;

			ProfileInformation profile(lowerPt, upperPt);
			profileMap.insert(std::pair<int,ProfileInformation>(index, profile));

			// Reset beta[index] to value at mode
			ccd->setBeta(index, x0);
		}
	}

	gettimeofday(&time2, NULL);
	return calculateSeconds(time1, time2);
}

double CcdInterface::diagnoseModel(CyclicCoordinateDescent *ccd, ModelData *modelData,	
		double loadTime,
		double updateTime) {
    return diagnoseModelImpl(ccd, modelData, loadTime, updateTime);		
}

double CcdInterface::logModel(CyclicCoordinateDescent *ccd, ModelData *modelData,
	    ProfileInformationMap& profileMap, bool withASE) {
    return logModelImpl(ccd, modelData, profileMap, withASE);
}	    
	    
// double CmdLineCcdInterface::logModelImpl(CyclicCoordinateDescent *ccd, ModelData *modelData,
// 	    ProfileInformationMap& profileMap, bool withASE) {
// 
// 	using namespace bsccs;
// 	struct timeval time1, time2;
// 	gettimeofday(&time1, NULL);
// 
// 	bsccs::EstimationOutputWriter estimates(*ccd, *modelData);
// 	estimates.addBoundInformation(profileMap);
// 
// 	string fileName = getPathAndFileName(arguments, "est_");
// 	estimates.writeFile(fileName.c_str());
// 	
// 	gettimeofday(&time2, NULL);
// 	return calculateSeconds(time1, time2);				
// }

void CcdInterface::setZeroBetaAsFixed(CyclicCoordinateDescent *ccd) {
	for (int j = 0; j < ccd->getBetaSize(); ++j) {
		if (ccd->getBeta(j) == 0.0) {
			ccd->setFixedBeta(j, true);
		}
	}
}

double CcdInterface::fitModel(CyclicCoordinateDescent *ccd) {
	if (arguments.noiseLevel > SILENT) {
		cout << "Using prior: " << ccd->getPriorInfo() << endl;
	}

	struct timeval time1, time2;
	gettimeofday(&time1, NULL);

	ccd->update(arguments.maxIterations, arguments.convergenceType, arguments.tolerance);

	gettimeofday(&time2, NULL);

	return calculateSeconds(time1, time2);
}

double CcdInterface::runBoostrap(
		CyclicCoordinateDescent *ccd,
		ModelData *modelData,
		std::vector<real>& savedBeta) {
	struct timeval time1, time2;
	gettimeofday(&time1, NULL);

	BootstrapSelector selector(arguments.replicates, modelData->getPidVectorSTL(),
			SUBJECT, arguments.seed);
	BootstrapDriver driver(arguments.replicates, modelData);

	driver.drive(*ccd, selector, arguments);
	gettimeofday(&time2, NULL);

	driver.logResults(arguments, savedBeta, ccd->getConditionId());
	return calculateSeconds(time1, time2);
}


double CcdInterface::runFitMLEAtMode(CyclicCoordinateDescent* ccd) {
	std::cout << std::endl << "Estimating MLE at posterior mode" << std::endl;

	struct timeval time1, time2;
	gettimeofday(&time1, NULL);

	setZeroBetaAsFixed(ccd);
	ccd->setPriorType(NONE);
	fitModel(ccd);

	gettimeofday(&time2, NULL);
	return calculateSeconds(time1, time2);
}

double CcdInterface::runCrossValidation(CyclicCoordinateDescent *ccd, ModelData *modelData) {
	struct timeval time1, time2;
	gettimeofday(&time1, NULL);

	CrossValidationSelector selector(arguments.fold, modelData->getPidVectorSTL(),
			SUBJECT, arguments.seed);

	AbstractCrossValidationDriver* driver;
	if (arguments.useAutoSearchCV) {
		if (arguments.useHierarchy) {
			driver = new HierarchyAutoSearchCrossValidationDriver(*modelData, arguments.gridSteps, arguments.lowerLimit, arguments.upperLimit);
		} else {
			driver = new AutoSearchCrossValidationDriver(*modelData, arguments.gridSteps, arguments.lowerLimit, arguments.upperLimit);
		}
	} else {
		if (arguments.useHierarchy) {
			driver = new HierarchyGridSearchCrossValidationDriver(arguments.gridSteps, arguments.lowerLimit, arguments.upperLimit);
		} else {
			driver = new GridSearchCrossValidationDriver(arguments.gridSteps, arguments.lowerLimit, arguments.upperLimit);
		}
	}

	driver->drive(*ccd, selector, arguments);

	gettimeofday(&time2, NULL);

	driver->logResults(arguments);

	if (arguments.doFitAtOptimal) {
		std::cout << "Fitting model at optimal hyperparameter" << std::endl;
 		// Do full fit for optimal parameter
		driver->resetForOptimal(*ccd, selector, arguments);
		fitModel(ccd);
		if (arguments.fitMLEAtMode) {
			runFitMLEAtMode(ccd);
		}
	}
	delete driver;

	return calculateSeconds(time1, time2);
}

bool CcdInterface::includesOption(const string& line, const string& option) {
	size_t found = line.find(option);
	return found != string::npos;
}

// CmdLineCcdInterface::CmdLineCcdInterface(int argc, char* argv[]) {
//     std::vector<std::string> args;
// 	for (int i = 0; i < argc; i++)
// 		args.push_back(argv[i]);
// 	CcdInterface::parseCommandLine(args);
// }

// CmdLineCcdInterface::~CmdLineCcdInterface() {
//     // Do nothing
// }

// void CmdLineCcdInterface::parseCommandLine(int argc, char* argv[],
// 		CCDArguments &arguments) {
// 	std::vector<std::string> args;
// 	for (int i = 0; i < argc; i++)
// 		args.push_back(argv[i]);
// 	CcdInterface::parseCommandLine(args, arguments);
// }

} // namespace

