/*=========================================================================
  Program:   vv                     http://www.creatis.insa-lyon.fr/rio/vv

  Authors belong to: 
  - University of LYON              http://www.universite-lyon.fr/
  - Léon Bérard cancer center       http://oncora1.lyon.fnclcc.fr
  - CREATIS CNRS laboratory         http://www.creatis.insa-lyon.fr

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the copyright notices for more information.

  It is distributed under dual licence

  - BSD        See included LICENSE.txt file
  - CeCILL-B   http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
======================================================================-====*/
#ifndef clitkCalculateTREGenericFilter_txx
#define clitkCalculateTREGenericFilter_txx

/* =================================================
 * @file   clitkCalculateTREGenericFilter.txx
 * @author 
 * @date   
 * 
 * @brief 
 * 
 ===================================================*/


namespace clitk
{
  
  //-----------------------------
  // Read DVF
  //-----------------------------
  template<unsigned int Dimension, unsigned int Components >
  void 
  CalculateTREGenericFilter::ReadVectorFields(void)
  {
    
    typedef itk::Vector<float, Components> VectorType;
    typedef itk::Image<VectorType, Dimension> DeformationFieldType;

    typedef itk::ImageFileReader<DeformationFieldType> InputReaderType;    
    std::vector<typename DeformationFieldType::Pointer> dvfs;
    for (unsigned int i=0; i< m_ArgsInfo.vf_given; i++)
      {
	typename InputReaderType::Pointer reader = InputReaderType::New();
	reader->SetFileName( m_ArgsInfo.vf_arg[i]);
	if (m_Verbose) std::cout<<"Reading vector field "<< i <<"..."<<std::endl;
	reader->Update();
	dvfs.push_back( reader->GetOutput() );
      }
    
    ProcessVectorFields<Dimension, Components>(dvfs, m_ArgsInfo.vf_arg);
  }

  //-----------------------------
  // Process DVF
  //-----------------------------
  template<unsigned int Dimension, unsigned int Components >
  void 
  CalculateTREGenericFilter::ProcessVectorFields(std::vector<typename itk::Image<itk::Vector<float, Components>, Dimension>::Pointer > dvfs, char** filenames )
  {
    
    std::vector<std::string> new_filenames;
    for (unsigned int i=0;i<m_ArgsInfo.vf_given;i++)
      new_filenames.push_back(filenames[i]);
    UpdateWithDim<Dimension>(dvfs, new_filenames); 
  }

    
  //-------------------------------------------------------------------
  // Update with the number of dimensions
  //-------------------------------------------------------------------
  template<unsigned int Dimension>
  void 
  CalculateTREGenericFilter::UpdateWithDim(std::vector<typename itk::Image<itk::Vector<float, Dimension>, Dimension>::Pointer > dvfs,  std::vector<std::string> filenames)
  {
    if (m_Verbose) std::cout << "Image was detected to be "<<Dimension<<"D..."<<std::endl;

    //-----------------------------
    // Typedefs
    //-----------------------------
    typedef double ValueType;
    typedef std::vector<ValueType> MeasureListType;

    typedef itk::Point<double, Dimension> PointType;
    typedef clitk::List<PointType> PointListType;
    typedef clitk::Lists<PointType> PointListsType;

    typedef itk::Vector<double, Dimension> VectorType;
    typedef clitk::List<VectorType> VectorListType;
    typedef clitk::Lists<VectorType> VectorListsType;
    
    typedef itk::Vector<float, Dimension> DeformationVectorType;
    typedef itk::Image<DeformationVectorType, Dimension> DeformationFieldType;

    //-----------------------------
    // Number of inputs
    //-----------------------------
    unsigned int numberOfFields=filenames.size();
    unsigned int numberOfLists=m_ArgsInfo.input_given;
    if ( (numberOfLists!=numberOfFields)  &&  (numberOfLists!=1) )
      {
	std::cerr<<"Error: Number of lists (="<<numberOfLists<<") different from number of DVF's (="<<numberOfFields<<") !"<<std::endl;
	return;
      }
    else if (numberOfLists==1) 
      {
	if (m_Verbose) std::cout<<numberOfFields<<" DVFs and 1 point list given..."<<std::endl;
      }
    else 
      {
	if (m_Verbose) std::cout<<numberOfLists<<" point lists and DVFs given..."<<std::endl;
      }

    
    //-----------------------------
    // Input point lists
    //-----------------------------   
    PointListsType pointLists;
    unsigned int numberOfPoints=0;
    for (unsigned int i=0; i<numberOfFields; i++)
      {
	// Read the lists
	if (numberOfLists==1) 
	  pointLists.push_back(PointListType(m_ArgsInfo.input_arg[0], m_Verbose) );
	else
	  pointLists.push_back(PointListType(m_ArgsInfo.input_arg[i], m_Verbose) );


	// Verify the number of points
	if (i==0) numberOfPoints=pointLists[i].size();
	else 
	  {
	    if  (numberOfPoints!=pointLists[i].size())
	      {
		std::cerr<<"Size of first list ("<<numberOfPoints
			 <<") is different from size of list "<<i
			 <<" ("<<pointLists[i].size()<<")..."<<std::endl;
		return;
	      }
	  }
      }    
    

    PointListType referencePointList;
    if (m_Verbose) std::cout<<"Reference point list:"<<std::endl;
    referencePointList=PointListType(m_ArgsInfo.ref_arg, m_Verbose);
    if  (numberOfPoints!=referencePointList.size())
      {
	std::cerr<<"Size of the first list ("<<numberOfPoints
		 <<") is different from size of the reference list ("
		 << referencePointList.size() <<")..."<<std::endl;
	return;
      }

    
    //-----------------------------
    // Interpolator
    //-----------------------------
    typedef clitk::GenericVectorInterpolator<args_info_clitkCalculateTRE,DeformationFieldType, double> GenericVectorInterpolatorType;
    typename GenericVectorInterpolatorType::Pointer genericInterpolator=GenericVectorInterpolatorType::New();
    genericInterpolator->SetArgsInfo(m_ArgsInfo);
    typedef itk::VectorInterpolateImageFunction<DeformationFieldType, double> InterpolatorType; 
    typename InterpolatorType::Pointer interpolator=genericInterpolator->GetInterpolatorPointer();
    

    //=====================================================================================
    // Original distance between points
    //=====================================================================================
    VectorListsType originalDistanceLists(numberOfFields);
    
    //-----------------------------
    // Calculate original distances
    //-----------------------------
    PointType referencePoint;
    VectorType distance;
    for (unsigned int phaseIndex=0; phaseIndex<numberOfFields; phaseIndex++)
      {
	for (unsigned int pointIndex=0; pointIndex<numberOfPoints; pointIndex++)   
	  {
	    referencePoint=referencePointList[pointIndex];
	    distance=pointLists[phaseIndex][pointIndex]-referencePoint;
	    originalDistanceLists[phaseIndex].push_back(distance);
	  }
      }
	
 
    //-----------------------------
    // Statistics Original
    //-----------------------------
    typedef DeformationListStatisticsFilter<VectorType> StatisticsFilterType;
    typename StatisticsFilterType::Pointer statisticsFilter=StatisticsFilterType::New();
    
    // Statistics  (magnitude)
    MeasureListType oMeanList, oStdList, oMaxList;
    ValueType oMean, oStd, oMax;
    statisticsFilter->GetStatistics(originalDistanceLists, oMean, oStd, oMax, oMeanList, oStdList, oMaxList);

    // Statistics  (per component)
    VectorListType oMeanXYZList, oStdXYZList,oMaxXYZList;
    VectorType oMeanXYZ, oStdXYZ, oMaxXYZ;
    statisticsFilter->GetStatistics(originalDistanceLists, oMeanXYZ, oStdXYZ, oMaxXYZ, oMeanXYZList, oStdXYZList, oMaxXYZList);


    //-----------------------------
    // Output
    //-----------------------------
    std::vector<std::string> labels;
    labels.push_back("MeanX\tSDX");
    labels.push_back("MeanY\tSDY");
    labels.push_back("MeanZ\tSDZ");
    labels.push_back("MeanT\tSDT");
    labels.push_back("MaxX");
    labels.push_back("MaxY");
    labels.push_back("MaxZ");
    labels.push_back("MaxT");


    // Output to screen
    if(m_Verbose)
      {
	
	// Numbers of DVF
	std::cout<<"# Number\tDVF"<<std::endl;
	for (unsigned int phaseIndex=0; phaseIndex<numberOfFields; phaseIndex++)
	  std::cout<<phaseIndex<<"\t"<<filenames[phaseIndex]<<std::endl;

	
	std::cout<<std::endl;
	std::cout<<"=================================================="<<std::endl;
	std::cout<<"||       Original distance between points       ||"<<std::endl;
	std::cout<<"=================================================="<<std::endl;
	std::cout<<std::endl;

	std::cout<<std::setprecision(3);

	

	// Labels of the columns
	std::cout<<"#DVF\tMean\tSD\t"<<labels[0];
	for(unsigned int dim=1;dim<Dimension;dim++)	std::cout<<"\t"<< labels[dim];
	std::cout<<"\tMax \t"<<labels[4];
	for(unsigned int dim=1;dim<Dimension;dim++)	std::cout<<"\t"<< labels[dim+4];
	std::cout<<std::endl;
	for (unsigned int phaseIndex=0; phaseIndex<numberOfFields; phaseIndex++)
	  {
	    std::cout<<phaseIndex;
	    std::cout<<"\t"<<oMeanList[phaseIndex]<<"\t"<<oStdList[phaseIndex];
	    std::cout<<"\t"<<oMeanXYZList[phaseIndex][0]<<"\t"<<oStdXYZList[phaseIndex][0];
	    for(unsigned int dim=1;dim<Dimension;dim++)
	      std::cout<<"\t"<<oMeanXYZList[phaseIndex][dim]<<"\t"<<oStdXYZList[phaseIndex][dim];
	    std::cout<<"\t"<<oMaxList[phaseIndex];
	    std::cout<<"\t"<<oMaxXYZList[phaseIndex][0];
	    for(unsigned int dim=1;dim<Dimension;dim++)
	      std::cout<<"\t"<<oMaxXYZList[phaseIndex][dim];
	    std::cout<<std::endl;
	  }

	// General
	std::cout<<std::endl;
	std::cout<<"@";
	std::cout<<"\t"<<oMean<<"\t"<<oStd;
	std::cout<<"\t"<<oMeanXYZ[0]<<"\t"<<oStdXYZ[0];
	for(unsigned int dim=1;dim<Dimension;dim++)
	  std::cout<<"\t"<<oMeanXYZ[dim]<<"\t"<<oStdXYZ[dim];
	std::cout<<"\t"<<oMax;
	std::cout<<"\t"<<oMaxXYZ[0];
	for(unsigned int dim=1;dim<Dimension;dim++)
	  std::cout<<"\t"<<oMaxXYZ[dim];
	std::cout<<std::endl;
      }

    // Output to file
    if( m_ArgsInfo.general_given)
      {
	// Add to the file
	std::ofstream general(m_ArgsInfo.general_arg);


	// Numbers of DVF
	for (unsigned int phaseIndex=0; phaseIndex<numberOfFields; phaseIndex++)
	  general<<phaseIndex<<"\t"<< filenames[phaseIndex]<<std::endl;
	  
	general<<std::endl;
	general<<"=================================================="<<std::endl;
	general<<"||       Original distance between points       ||"<<std::endl;
	general<<"=================================================="<<std::endl;
	general<<std::endl;

	general<<std::setprecision(3);

	// Labels of the columns
	general<<"#DVF\tMean\tSD\t"<<labels[0];
	for(unsigned int dim=1;dim<Dimension;dim++)	general<<"\t"<< labels[dim];
	general<<"\tMax \t"<<labels[4];
	for(unsigned int dim=1;dim<Dimension;dim++)	general<<"\t"<< labels[dim+4];
	general<<std::endl;
	for (unsigned int phaseIndex=0; phaseIndex<numberOfFields; phaseIndex++)
	  {
	    general<<phaseIndex;
	    general<<"\t"<<oMeanList[phaseIndex]<<"\t"<<oStdList[phaseIndex];
	    general<<"\t"<<oMeanXYZList[phaseIndex][0]<<"\t"<<oStdXYZList[phaseIndex][0];
	    for(unsigned int dim=1;dim<Dimension;dim++)
	      general<<"\t"<<oMeanXYZList[phaseIndex][dim]<<"\t"<<oStdXYZList[phaseIndex][dim];
	    general<<"\t"<<oMaxList[phaseIndex];
	    general<<"\t"<<oMaxXYZList[phaseIndex][0];
	    for(unsigned int dim=1;dim<Dimension;dim++)
	      general<<"\t"<<oMaxXYZList[phaseIndex][dim];
	    general<<std::endl;
	  }

	// General
	general<<std::endl;
	general<<"@";
	general<<"\t"<<oMean<<"\t"<<oStd;
	general<<"\t"<<oMeanXYZ[0]<<"\t"<<oStdXYZ[0];
	for(unsigned int dim=1;dim<Dimension;dim++)
	  general<<"\t"<<oMeanXYZ[dim]<<"\t"<<oStdXYZ[dim];
	general<<"\t"<<oMax;
	general<<"\t"<<oMaxXYZ[0];
	for(unsigned int dim=1;dim<Dimension;dim++)
	  general<<"\t"<<oMaxXYZ[dim];
	general<<std::endl;
 	general.close();
      }


    //=====================================================================================
    // Distance between points after warp
    //=====================================================================================
    PointListsType warpedPointLists(numberOfFields);
    VectorListsType treLists(numberOfFields), displacementLists(numberOfFields);
    
    //-----------------------------
    // Calculate residual distances
    //-----------------------------
    VectorType displacement, tre;
    PointType  warpedPoint;

    for (unsigned int phaseIndex=0; phaseIndex<numberOfFields; phaseIndex++)
      { 
	interpolator->SetInputImage( dvfs[phaseIndex]);
	for (unsigned int pointIndex=0; pointIndex<numberOfPoints; pointIndex++)   
	  {
	    // Reference
	    referencePoint=referencePointList[pointIndex];

	    // Inside?
	    if ( interpolator->IsInsideBuffer(referencePoint) )
	      displacement=interpolator->Evaluate(referencePoint); 
	    else
	      displacement.Fill(0.0); 

	    // Warp
	    warpedPoint=referencePoint+displacement;
	    displacementLists[phaseIndex].push_back(displacement);
	    warpedPointLists[phaseIndex].push_back(warpedPoint);
	    tre=pointLists[phaseIndex][pointIndex]-warpedPoint;
	    treLists[phaseIndex].push_back(tre);
	  }
      }


    //-----------------------------
    // Statistics displacements
    //-----------------------------
    
    // Statistics  (magnitude)
    MeasureListType dmeanList, dstdList, dmaxList;
    ValueType dmean, dstd, dmax;
    statisticsFilter->GetStatistics(displacementLists, dmean, dstd, dmax, dmeanList, dstdList, dmaxList);

    // Statistics  (per component)
    VectorListType dmeanXYZList, dstdXYZList,dmaxXYZList;
    VectorType dmeanXYZ, dstdXYZ, dmaxXYZ;
    statisticsFilter->GetStatistics(displacementLists, dmeanXYZ, dstdXYZ, dmaxXYZ,dmeanXYZList, dstdXYZList, dmaxXYZList);


    //-----------------------------
    // Statistics TRE
    //-----------------------------
    
    // Statistics  (magnitude)
    MeasureListType meanList, stdList, maxList;
    ValueType mean, std, max;
    statisticsFilter->GetStatistics(treLists, mean, std, max, meanList, stdList, maxList);

    // Statistics  (per component)
    VectorListType meanXYZList, stdXYZList,maxXYZList;
    VectorType meanXYZ, stdXYZ, maxXYZ;
    statisticsFilter->GetStatistics(treLists, meanXYZ, stdXYZ, maxXYZ, meanXYZList, stdXYZList, maxXYZList);


    // Output to screen
    if(m_Verbose)
      {

	std::cout<<std::endl;
	std::cout<<"=================================================="<<std::endl;
	std::cout<<"||       Residual distance between points       ||"<<std::endl;
	std::cout<<"=================================================="<<std::endl;
	std::cout<<std::endl;

	std::cout<<std::setprecision(3);

	// Labels of the columns
	std::cout<<"#DVF\tMean\tSD\t"<<labels[0];
	for(unsigned int dim=1;dim<Dimension;dim++)	std::cout<<"\t"<< labels[dim];
	std::cout<<"\tMax \t"<<labels[4];
	for(unsigned int dim=1;dim<Dimension;dim++)	std::cout<<"\t"<< labels[dim+4];
	std::cout<<std::endl;
	for (unsigned int phaseIndex=0; phaseIndex<numberOfFields; phaseIndex++)
	  {
	    std::cout<<phaseIndex;
	    std::cout<<"\t"<<meanList[phaseIndex]<<"\t"<<stdList[phaseIndex];
	    std::cout<<"\t"<<meanXYZList[phaseIndex][0]<<"\t"<<stdXYZList[phaseIndex][0];
	    for(unsigned int dim=1;dim<Dimension;dim++)
	      std::cout<<"\t"<<meanXYZList[phaseIndex][dim]<<"\t"<<stdXYZList[phaseIndex][dim];
	    std::cout<<"\t"<<maxList[phaseIndex];
	    std::cout<<"\t"<<maxXYZList[phaseIndex][0];
	    for(unsigned int dim=1;dim<Dimension;dim++)
	      std::cout<<"\t"<<maxXYZList[phaseIndex][dim];
	    std::cout<<std::endl;
	  }

	// General
	std::cout<<std::endl;
	std::cout<<"@";
	std::cout<<"\t"<<mean<<"\t"<<std;
	std::cout<<"\t"<<meanXYZ[0]<<"\t"<<stdXYZ[0];
	for(unsigned int dim=1;dim<Dimension;dim++)
	  std::cout<<"\t"<<meanXYZ[dim]<<"\t"<<stdXYZ[dim];
	std::cout<<"\t"<<max;
	std::cout<<"\t"<<maxXYZ[0];
	for(unsigned int dim=1;dim<Dimension;dim++)
	  std::cout<<"\t"<<maxXYZ[dim];
	std::cout<<std::endl;
      }

    // Output to file
    if( m_ArgsInfo.general_given)
      {
	// Add to the file
	std::ofstream general(m_ArgsInfo.general_arg, ios_base::app);

	general<<std::endl;
	general<<"=================================================="<<std::endl;
	general<<"||       Residual distance between points       ||"<<std::endl;
	general<<"=================================================="<<std::endl;
	general<<std::endl;

	general<<std::setprecision(3);

	// Labels of the columns
	general<<"#DVF\tMean\tSD\t"<<labels[0];
	for(unsigned int dim=1;dim<Dimension;dim++)	general<<"\t"<< labels[dim];
	general<<"\tMax \t"<<labels[4];
	for(unsigned int dim=1;dim<Dimension;dim++)	general<<"\t"<< labels[dim+4];
	general<<std::endl;
	for (unsigned int phaseIndex=0; phaseIndex<numberOfFields; phaseIndex++)
	  {
	    general<<phaseIndex;
	    general<<"\t"<<meanList[phaseIndex]<<"\t"<<stdList[phaseIndex];
	    general<<"\t"<<meanXYZList[phaseIndex][0]<<"\t"<<stdXYZList[phaseIndex][0];
	    for(unsigned int dim=1;dim<Dimension;dim++)
	      general<<"\t"<<meanXYZList[phaseIndex][dim]<<"\t"<<stdXYZList[phaseIndex][dim];
	    general<<"\t"<<maxList[phaseIndex];
	    general<<"\t"<<maxXYZList[phaseIndex][0];
	    for(unsigned int dim=1;dim<Dimension;dim++)
	      general<<"\t"<<maxXYZList[phaseIndex][dim];
	    general<<std::endl;
	  }

	// General
	general<<std::endl;
	general<<"@";
	general<<"\t"<<mean<<"\t"<<std;
	general<<"\t"<<meanXYZ[0]<<"\t"<<stdXYZ[0];
	for(unsigned int dim=1;dim<Dimension;dim++)
	  general<<"\t"<<meanXYZ[dim]<<"\t"<<stdXYZ[dim];
	general<<"\t"<<max;
	general<<"\t"<<maxXYZ[0];
	for(unsigned int dim=1;dim<Dimension;dim++)
	  general<<"\t"<<maxXYZ[dim];
	general<<std::endl;
 	general.close();
      }

    // Output original points
    if( m_ArgsInfo.original_given)
      {

	std::vector<std::string> filenames;
	for (unsigned int i=0;i<numberOfFields;i++)
	  {
	    std::ostringstream number_ostr;
	    number_ostr << i;
	    std::string number_str =  number_ostr.str();
	    filenames.push_back(m_ArgsInfo.original_arg+number_str);
	  }
	originalDistanceLists.Write(filenames, m_Verbose );
      }


    // Output original magnitude points
    if( m_ArgsInfo.originalMag_given)
      {
	clitk::Lists<itk::FixedArray<double,1> > originalDistanceListsMag=originalDistanceLists.Norm();
	std::vector<std::string> filenames;
	for (unsigned int i=0;i<numberOfFields;i++)
	  {
	    std::ostringstream number_ostr;
	    number_ostr << i;
	    std::string number_str =  number_ostr.str();
	    filenames.push_back(m_ArgsInfo.originalMag_arg+number_str);
	  }
	originalDistanceListsMag.Write(filenames, m_Verbose );
      }

    // Output displacement
    if( m_ArgsInfo.displacement_given)
      {

	std::vector<std::string> filenames;
	for (unsigned int i=0;i<numberOfFields;i++)
	  {
	    std::ostringstream number_ostr;
	    number_ostr << i;
	    std::string number_str =  number_ostr.str();
	    filenames.push_back(m_ArgsInfo.displacement_arg+number_str);
	  }
	displacementLists.Write(filenames, m_Verbose );
      }


    // Output displacement magnitude 
    if( m_ArgsInfo.displacementMag_given)
      {
	clitk::Lists<itk::FixedArray<double,1> > displacementListsMag=displacementLists.Norm();
	std::vector<std::string> filenames;
	for (unsigned int i=0;i<numberOfFields;i++)
	  {
	    std::ostringstream number_ostr;
	    number_ostr << i;
	    std::string number_str =  number_ostr.str();
	    filenames.push_back(m_ArgsInfo.displacementMag_arg+number_str);
	  }
	displacementListsMag.Write(filenames, m_Verbose );
      }

    // Output warped points
    if( m_ArgsInfo.warp_given)
      {

	std::vector<std::string> filenames;
	for (unsigned int i=0;i<m_ArgsInfo.warp_given;i++)
	  {
	    std::ostringstream number_ostr;
	    number_ostr << i;
	    std::string number_str =  number_ostr.str();
	    filenames.push_back(m_ArgsInfo.warp_arg+number_str);
	  }
	warpedPointLists.Write(filenames, m_Verbose );
      }

    // Output tre 
    if( m_ArgsInfo.tre_given)
      {

	std::vector<std::string> filenames;
	for (unsigned int i=0;i<numberOfFields;i++)
	  {
	    std::ostringstream number_ostr;
	    number_ostr << i;
	    std::string number_str =  number_ostr.str();
	    filenames.push_back(m_ArgsInfo.tre_arg+number_str);
	  }
	treLists.Write(filenames, m_Verbose );
      }

    // Output tre mag
    if( m_ArgsInfo.treMag_given)
      {
	clitk::Lists<itk::FixedArray<double,1> > treMagLists=treLists.Norm();

	std::vector<std::string> filenames;
	for (unsigned int i=0;i<numberOfFields;i++)
	  {
	    std::ostringstream number_ostr;
	    number_ostr << i;
	    std::string number_str =  number_ostr.str();
	    filenames.push_back(m_ArgsInfo.treMag_arg+number_str);
	  }
	treMagLists.Write(filenames, m_Verbose );
      }
	
  }


}//end clitk
 
#endif //#define clitkCalculateTREGenericFilter_txx
