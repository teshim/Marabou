/*********************                                                        */
/*! \file Marabou.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

#include "AcasParser.h"
#include "GlobalConfiguration.h"
#include "File.h"
#include "MStringf.h"
#include "Marabou.h"
#include "Options.h"
#include "PropertyParser.h"
#include "MarabouError.h"
#include "QueryLoader.h"

#include "GammaUnsat.h"
#include "ResidualReasoningSplitProvider.h"
#include "PiecewiseLinearCaseSplitUtils.h"

#ifdef _WIN32
#undef ERROR
#endif

Marabou::Marabou()
    : _acasParser( NULL )
    , _engine( std::make_shared<SplitProvidersManager>() )
{
}

Marabou::~Marabou()
{
    if ( _acasParser )
    {
        delete _acasParser;
        _acasParser = NULL;
    }
}

void test1();
void test2();

void Marabou::run()
{
    // printf( "**** start test1 ****\n" );
    // test1();
    // printf( "\n\n" );
    // printf( "**** start test2 ****\n" );
    // test2();
    // printf( "\n\n" );
    // return;
    struct timespec start = TimeUtils::sampleMicro();

    // temporay code for debug purposes code
    auto inputFile = Options::get()->getString( Options::GAMMA_UNSAT_INPUT_FILE );
    auto outputFile = Options::get()->getString( Options::GAMMA_UNSAT_OUTPUT_FILE );
    GammaUnsat gammaUnsat;
    if ( inputFile.length() > 0 && File::exists( inputFile ) ) {
        gammaUnsat = GammaUnsat::readFromFile( inputFile );
    }

    auto residualReasoner = std::make_shared<ResidualReasoningSplitProvider>( gammaUnsat );
    _engine.addSplitProvider( residualReasoner );

    prepareInputQuery();
    solveQuery();

    struct timespec end = TimeUtils::sampleMicro();

    unsigned long long totalElapsed = TimeUtils::timePassed( start, end );
    displayResults( totalElapsed );

    if ( outputFile.length() > 0 )
        residualReasoner->gammaUnsat().saveToFile( outputFile );
}

#pragma GCC push_options
#pragma GCC optimize ("O0")

char const* activationStr( ActivationType activation )
{
    switch ( activation )
    {
    case ActivationType::ACTIVE:
        return "Active";
    case ActivationType::INACTIVE:
        return "Inactive";
    }
    throw std::runtime_error( "inavlid input to function activationStr " + std::to_string( static_cast<int>( activation ) ) );
}

void test1()
{
    // /* 
    // gammaUnsat = [{c1:1, c2:1}, {c1:1, c3:-1}, {c2:-1,c3:-1}] and
    // _pastSplits = {c1:1}
    // then 2 splits: {c2:-1, c3:1} are derived from the 2 first clauses
    // */
    // GammaUnsat gamma = GammaUnsat::readFromFile( "/home/elazar/marabou/Marabou/gamma.txt" );
    GammaUnsat gamma = GammaUnsat();
    GammaUnsat::UnsatSequence seq1 = GammaUnsat::UnsatSequence();
    seq1.activations.append( PLCaseSplitRawData( 1, 10, ActivationType::ACTIVE ) );
    seq1.activations.append( PLCaseSplitRawData( 2, 20, ActivationType::ACTIVE ) );
    gamma.addUnsatSequence( seq1 );
    GammaUnsat::UnsatSequence seq2 = GammaUnsat::UnsatSequence();
    seq2.activations.append( PLCaseSplitRawData( 1, 10, ActivationType::ACTIVE ) );
    seq2.activations.append( PLCaseSplitRawData( 3, 30, ActivationType::INACTIVE ) );
    gamma.addUnsatSequence( seq2 );
    GammaUnsat::UnsatSequence seq3 = GammaUnsat::UnsatSequence();
    seq3.activations.append( PLCaseSplitRawData( 2, 20, ActivationType::INACTIVE ) );
    seq3.activations.append( PLCaseSplitRawData( 3, 30, ActivationType::INACTIVE ) );
    gamma.addUnsatSequence( seq3 );

    ResidualReasoningSplitProvider provider = ResidualReasoningSplitProvider( gamma );
    // provider.gammaUnsat().saveToFile( "/home/elazar/marabou/Marabou/gamma2.txt" );
    // return;

    // at start - no need to split
    if ( provider.needToSplit() ) {
        printf( "at start, provider.needToSplit() != nullpoint" );
    }
    // activate split1: variable = 1, lb = 0;
    PiecewiseLinearCaseSplit split1 = ReluConstraint( 1, 10 ).getActiveSplit();
    provider.onSplitPerformed( SplitInfo( split1 ) );

    const auto shouldBeASplit2 = provider.needToSplit();
    if ( !shouldBeASplit2 )
    {
        printf( "no split. a bug\n" );
        return;
    }
    printf( "1 split on %d activation=%s\n", shouldBeASplit2->reluRawData()->_b, activationStr( shouldBeASplit2->reluRawData()->_activation ) );
    provider.onSplitPerformed( SplitInfo( *shouldBeASplit2 ) );

    const auto shouldBeASplit3 = provider.needToSplit();
    if ( !shouldBeASplit3 )
    {
        printf( "no split. a bug\n" );
        return;
    }
    printf( "2 split on %d activation=%s\n", shouldBeASplit3->reluRawData()->_b, activationStr( shouldBeASplit3->reluRawData()->_activation ) );
    provider.onSplitPerformed( SplitInfo( *shouldBeASplit3 ) );
}

void test2()
{
    /*
    sequential derivation (at first c1:1 -> c2:-1, and then c2:-1 -> c3:1)
    gammaUnsat = [{c1:1, c2:1}, {c2:-1,c3:-1}] and
    _pastSplits = {c1:1}
    then 2 splits: {c2:-1, c3:1} are derived from the 2 first clauses
    */
    GammaUnsat gamma = GammaUnsat();
    GammaUnsat::UnsatSequence seq1 = GammaUnsat::UnsatSequence();
    seq1.activations.append( PLCaseSplitRawData( 1, 10, ActivationType::ACTIVE ) );
    seq1.activations.append( PLCaseSplitRawData( 2, 20, ActivationType::ACTIVE ) );
    gamma.addUnsatSequence( seq1 );
    GammaUnsat::UnsatSequence seq2 = GammaUnsat::UnsatSequence();
    seq2.activations.append( PLCaseSplitRawData( 2, 20, ActivationType::INACTIVE ) );
    seq2.activations.append( PLCaseSplitRawData( 3, 30, ActivationType::INACTIVE ) );
    gamma.addUnsatSequence( seq2 );

    ResidualReasoningSplitProvider provider = ResidualReasoningSplitProvider( gamma );

    // at start - no need to split
    if ( provider.needToSplit() ) {
        printf( "at start, provider.needToSplit() != nullpoint\n" );
        return;
    }
    // activate split1: variable = 1, lb = 0;
    PiecewiseLinearCaseSplit split1 = ReluConstraint( 1, 10 ).getActiveSplit();
    provider.onSplitPerformed( SplitInfo( split1 ) );

    const auto shouldBeASplit2 = provider.needToSplit();
    if ( !shouldBeASplit2 )
    {
        printf( "no split. a bug\n" );
        return;
    }
    printf( "3 split on %d activation=%s\n", shouldBeASplit2->reluRawData()->_b, activationStr( shouldBeASplit2->reluRawData()->_activation ) );
    provider.onSplitPerformed( SplitInfo( *shouldBeASplit2 ) );

    const auto shouldBeASplit3 = provider.needToSplit();
    if ( !shouldBeASplit3 )
    {
        printf( "no split. a bug\n" );
        return;
    }
    printf( "4 split on %d activation=%s\n", shouldBeASplit3->reluRawData()->_b, activationStr( shouldBeASplit3->reluRawData()->_activation ) );
    provider.onSplitPerformed( SplitInfo( *shouldBeASplit3 ) );
}

#pragma GCC pop_options

void Marabou::prepareInputQuery()
{
    String inputQueryFilePath = Options::get()->getString( Options::INPUT_QUERY_FILE_PATH );
    if ( inputQueryFilePath.length() > 0 )
    {
        /*
          Step 1: extract the query
        */
        if ( !File::exists( inputQueryFilePath ) )
        {
            printf( "Error: the specified inputQuery file (%s) doesn't exist!\n", inputQueryFilePath.ascii() );
            throw MarabouError( MarabouError::FILE_DOESNT_EXIST, inputQueryFilePath.ascii() );
        }

        printf( "InputQuery: %s\n", inputQueryFilePath.ascii() );
        _inputQuery = QueryLoader::loadQuery( inputQueryFilePath );
        _inputQuery.constructNetworkLevelReasoner();
    }
    else
    {
        /*
          Step 1: extract the network
        */
        String networkFilePath = Options::get()->getString( Options::INPUT_FILE_PATH );
        if ( !File::exists( networkFilePath ) )
        {
            printf( "Error: the specified network file (%s) doesn't exist!\n", networkFilePath.ascii() );
            throw MarabouError( MarabouError::FILE_DOESNT_EXIST, networkFilePath.ascii() );
        }
        printf( "Network: %s\n", networkFilePath.ascii() );

        // For now, assume the network is given in ACAS format
        _acasParser = new AcasParser( networkFilePath );
        _acasParser->generateQuery( _inputQuery );
        _inputQuery.constructNetworkLevelReasoner();

        /*
          Step 2: extract the property in question
        */
        String propertyFilePath = Options::get()->getString( Options::PROPERTY_FILE_PATH );
        if ( propertyFilePath != "" )
        {
            printf( "Property: %s\n", propertyFilePath.ascii() );
            PropertyParser().parse( propertyFilePath, _inputQuery );
        }
        else
            printf( "Property: None\n" );

        printf( "\n" );
    }

    String queryDumpFilePath = Options::get()->getString( Options::QUERY_DUMP_FILE );
    if ( queryDumpFilePath.length() > 0 )
    {
        _inputQuery.saveQuery( queryDumpFilePath );
        printf( "\nInput query successfully dumped to file\n" );
        exit( 0 );
    }
}

void Marabou::solveQuery()
{
    if ( _engine.processInputQuery( _inputQuery ) )
        _engine.solve( Options::get()->getInt( Options::TIMEOUT ) );

    if ( _engine.getExitCode() == Engine2::SAT )
        _engine.extractSolution( _inputQuery );
}

void Marabou::displayResults( unsigned long long microSecondsElapsed ) const
{
    Engine2::ExitCode result = _engine.getExitCode();
    String resultString;

    if ( result == Engine2::UNSAT )
    {
        resultString = "unsat";
        printf( "unsat\n" );
    }
    else if ( result == Engine2::SAT )
    {
        resultString = "sat";
        printf( "sat\n" );

        printf( "Input assignment:\n" );
        for ( unsigned i = 0; i < _inputQuery.getNumInputVariables(); ++i )
            printf( "\tx%u = %lf\n", i, _inputQuery.getSolutionValue( _inputQuery.inputVariableByIndex( i ) ) );

        if ( _inputQuery._networkLevelReasoner )
        {
            double* input = new double[_inputQuery.getNumInputVariables()];
            for ( unsigned i = 0; i < _inputQuery.getNumInputVariables(); ++i )
                input[i] = _inputQuery.getSolutionValue( _inputQuery.inputVariableByIndex( i ) );

            NLR::NetworkLevelReasoner* nlr = _inputQuery._networkLevelReasoner;
            NLR::Layer* lastLayer = nlr->getLayer( nlr->getNumberOfLayers() - 1 );
            double* output = new double[lastLayer->getSize()];

            nlr->evaluate( input, output );

            printf( "\n" );
            printf( "Output:\n" );
            for ( unsigned i = 0; i < lastLayer->getSize(); ++i )
                printf( "\ty%u = %lf\n", i, output[i] );
            printf( "\n" );
            delete[] input;
            delete[] output;
        }
        else
        {
            printf( "\n" );
            printf( "Output:\n" );
            for ( unsigned i = 0; i < _inputQuery.getNumOutputVariables(); ++i )
                printf( "\ty%u = %lf\n", i, _inputQuery.getSolutionValue( _inputQuery.outputVariableByIndex( i ) ) );
            printf( "\n" );
        }
    }
    else if ( result == Engine2::TIMEOUT )
    {
        resultString = "TIMEOUT";
        printf( "Timeout\n" );
    }
    else if ( result == Engine2::ERROR )
    {
        resultString = "ERROR";
        printf( "Error\n" );
    }
    else
    {
        resultString = "UNKNOWN";
        printf( "UNKNOWN EXIT CODE! (this should not happen)" );
    }

    // Create a summary file, if requested
    String summaryFilePath = Options::get()->getString( Options::SUMMARY_FILE );
    if ( summaryFilePath != "" )
    {
        File summaryFile( summaryFilePath );
        summaryFile.open( File::MODE_WRITE_TRUNCATE );

        // Field #1: result
        summaryFile.write( resultString );

        // Field #2: total elapsed time
        summaryFile.write( Stringf( " %u ", microSecondsElapsed / 1000000 ) ); // In seconds

        // Field #3: number of visited tree states
        summaryFile.write( Stringf( "%u ",
            _engine.getStatistics()->getNumVisitedTreeStates() ) );

        // Field #4: average pivot time in micro seconds
        summaryFile.write( Stringf( "%u",
            _engine.getStatistics()->getAveragePivotTimeInMicro() ) );

        summaryFile.write( "\n" );
    }
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
