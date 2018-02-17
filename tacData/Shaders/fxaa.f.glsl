#version 430

in vec2 uvs;
out vec4 outColor;
uniform sampler2D quadTexture;
uniform vec2 rcpFrame; // ( 1.0 / frameWidth, 1.0 / frameHeight )

#define APPLY_FXAA 1

void FXAA_DEFINES()
{
  // FXAA_EDGE_THRESHOLD 
  //   The minimum amount of local contrast required to apply algorithm.
  //     1.0/3.0  - too little
  //     1.0/4.0  - good start
  //     1.0/8.0  - applies to more edges
  //     1.0/16.0 - overkill
  //
  // FXAA_EDGE_THRESHOLD_MIN
  //   Trims the algorithm from processing darks.
  //   Perf optimization.
  //     1.0/32.0 - visible limit (smaller isn't visible)
  //     1.0/16.0 - good compromise
  //     1.0/12.0 - upper limit (seeing artifacts)
  //
  // FXAA_SEARCH_STEPS
  //   Maximum number of search steps for end of span.
  //
  // FXAA_SEARCH_ACCELERATION
  //   How much to accelerate search,
  //     1 - no acceleration
  //     2 - skip by 2 pixels
  //     3 - skip by 3 pixels
  //     4 - skip by 4 pixels
  //
  // FXAA-SEARCH_THRESHOLD
  //   Controls when to stop searching.
  //     1.0/4.0 - seems to be the best quality wise
  //
  // FXAA_SUBPIX_FASTER
  //   Turn on lower quality but faster subpix path.
  //   Not recomended, but used in preset 0.
  //
  // FXAA_SUBPIX
  //   Toggle subpix filtering.
  //     0 - turn off
  //     1 - turn on
  //     2 - turn on full (ignores FXAA_SUBPIX_TRIM and CAP)
  //
  // FXAA_SUBPIX_TRIM
  //   Controls sub-pixel aliasing removal.
  //     1.0/2.0 - low removal
  //     1.0/3.0 - medium removal
  //     1.0/4.0 - default removal
  //     1.0/8.0 - high removal
  //     0.0 - complete removal
  //
  // FXAA_SUBPIX_CAP
  //   Insures fine detail is not completely removed.
  //   This is important for the transition of sub-pixel detail,
  //   like fences and wires.
  //     3.0/4.0 - default (medium amount of filtering)
  //     7.0/8.0 - high amount of filtering
  //     1.0 - no capping of sub-pixel aliasing removal
}

// preset 3
#define FXAA_EDGE_THRESHOLD      ( 1.0 / 8.0 )
#define FXAA_EDGE_THRESHOLD_MIN  ( 1.0 / 24.0 )
#define FXAA_SEARCH_STEPS        16
#define FXAA_SEARCH_ACCELERATION 1
#define FXAA_SEARCH_THRESHOLD    ( 1.0 / 4.0 )
#define FXAA_SUBPIX              1
#define FXAA_SUBPIX_FASTER       0
#define FXAA_SUBPIX_CAP          ( 3.0 / 4.0 )
#define FXAA_SUBPIX_TRIM         ( 1.0 / 6.0 )

// always
#define FXAA_SUBPIX_TRIM_SCALE ( 1.0 / ( 1.0 - FXAA_SUBPIX_TRIM ) )


// Return the luma, the estimation of luminance from rgb inputs.
// This approximates luma using one FMA instruction,
// skipping normalization and tossing out blue.
// Luma() will range 0.0 to 2.963210702.
float Luma( vec3 rgb )
{
  // luma  .299 R'+.587 G'+.114 B'   [does not to match chromaticities]
  return rgb.y * ( 0.587 / 0.299 ) + rgb.x;
}


void DEBUG_KNOBS()
{
  // All debug knobs draw FXAA-untouched pixels in FXAA computed
  // luma (monochrome).
  //
  // FXAA_DEBUG_PASSTHROUGH
  //   Red for pixels which are filtered by FXAA with a
  //   yellow tint on sub-pixel aliasing filtered by FXAA.
  //
  // FXAA_DEBUG_HORZVERT
  //   Blue for horizontal edges, gold for vertical edges.
  //
  // FXAA_DEBUG_PAIR
  //   Blue/green for the 2 pixel pair choice.
  //
  // FXAA_DEBUG_NEGPOS
  //   Red/blue for which side of center of span.
  //
  // FXAA_DEBUG_OFFSET
  //   Red/blue for -/+ x, gold/skyblue for -/+ y.
}

#define FXAA_DEBUG_PASSTHROUGH 0
#define FXAA_DEBUG_HORZVERT    0
#define FXAA_DEBUG_PAIR        0
#define FXAA_DEBUG_NEGPOS      0
#define FXAA_DEBUG_OFFSET      0

#if FXAA_DEBUG_PASSTHROUGH || FXAA_DEBUG_HORZVERT || FXAA_DEBUG_PAIR
#define FXAA_DEBUG 1
#endif
#if FXAA_DEBUG_NEGPOS || FXAA_DEBUG_OFFSET
#define FXAA_DEBUG 1
#endif
#ifndef FXAA_DEBUG
#define FXAA_DEBUG 0
#endif


vec3 FxaaPixelShader(
    // Output of FxaaVertexShader interpolated across screen.
    //  xy -> actual texture position {0.0 to 1.0}
    vec2 pos,

    // Input texture.
    sampler2D tex,

    // {1.0/frameWidth, 1.0/frameHeight}
    vec2 rcpFrame )
{
  //////////////////////////////////////////////////////////
  // EARLY EXIT IF LOCAL CONTRAST BELOW EDGE DETECT LIMIT //
  //////////////////////////////////////////////////////////
  {
    // Majority of pixels of a typical image do not require filtering, 
    // often pixels are grouped into blocks which could benefit from early exit 
    // right at the beginning of the algorithm. 
    // Given the following neighborhood, 
    // 
    //       N   
    //     W M E
    //       S   
    //
    // If the difference in local maximum and minimum luma (contrast "range") 
    // is lower than a threshold proportional to the
    // maximum local luma ("rangeMax"), 
    // then the shader early exits (no visible aliasing). 
    // This threshold is clamped at a minimum value ("FXAA_EDGE_THRESHOLD_MIN")
    // to avoid processing in really dark areas.    
  }

  vec3 rgbN = textureOffset( tex, pos, ivec2(  0,  1 ) ).xyz;
  vec3 rgbW = textureOffset( tex, pos, ivec2( -1,  0 ) ).xyz;
  vec3 rgbM = textureOffset( tex, pos, ivec2(  0,  0 ) ).xyz;
  vec3 rgbE = textureOffset( tex, pos, ivec2(  1,  0 ) ).xyz;
  vec3 rgbS = textureOffset( tex, pos, ivec2(  0, -1 ) ).xyz;
  float lumaN = Luma( rgbN );
  float lumaW = Luma( rgbW );
  float lumaM = Luma( rgbM );
  float lumaE = Luma( rgbE );
  float lumaS = Luma( rgbS );

  float rangeMin = min( min( min( min(
            lumaN, lumaW ), lumaM ), lumaE ), lumaS );
  float rangeMax = max( max( max( max(
            lumaN, lumaW ), lumaM ), lumaE ), lumaS );

  float range = rangeMax - rangeMin;

#if FXAA_DEBUG
  float lumaO = lumaM / ( 1.0 + ( 0.587 / 0.299 ) );
#endif

  // if the pixel is skipped
  if( range < max(
        FXAA_EDGE_THRESHOLD_MIN,
        rangeMax * FXAA_EDGE_THRESHOLD ) )
  {
#if FXAA_DEBUG
    // color it by it's luma
    return vec3( lumaO );

    // color it by luma ( red = luma = 0, yellow: luma = 1 )
    //return vec3( 1, lumaO, 0 );
#endif
    // pixel is skipped, so return the pixel color
    return vec3( rgbM );
  }

#if FXAA_SUBPIX > 0
#if FXAA_SUBPIX_FASTER
  vec3 rgbL = ( rgbN + rgbW + rgbE + rgbS + rgbM ) * vec3( 1.0 / 5.0 );
#else
  vec3 rgbL = rgbN + rgbW + rgbM + rgbE + rgbS;
#endif
#endif

  /////////////////////
  // COMPUTE LOWPASS //
  /////////////////////
  {
    // FXAA computes a local neighborhood lowpass value as follows,
    //
    //   (N + W + E + S)/4
    //
    // Then uses the ratio of the contrast range of the lowpass
    // and the range found in the early exit check,
    // as a sub-pixel aliasing detection filter.
    // When FXAA detects sub-pixel aliasing (such as single pixel dots),
    // it later blends in "blendL" amount
    // of a lowpass value (computed in the next section) to the final result.
    //
    // From the FXAA_WhitePaper.pdf:
    //   Pixel contrast is estimated as the absolute difference in pixel luma
    //   from a lowpass luma (computed as the average of the North, South,
    //   East and West neighbors).
    //
    //   The ratio of pixel contrast to local contrast is used to detect
    //   sub-pixel aliasing.
    //
    //   This ratio approaches 1.0 in the presence of single pixel dots
    //   and otherwise begins to fall off towards 0.0 as more pixels
    //   contribute to an edge.
    //
    //   The ratio is transformed into the amount of lowpass filter to blend
    //   in at the end of the algorithm.
  }
#if FXAA_SUBPIX != 0
  float lumaL = ( lumaN + lumaW + lumaE + lumaS ) * 0.25;
  float rangeL = abs( lumaL - lumaM );
#endif
#if FXAA_SUBPIX == 1
  float blendL = max( 0.0,
      ( rangeL / range ) - FXAA_SUBPIX_TRIM ) * FXAA_SUBPIX_TRIM_SCALE;
  blendL = min( FXAA_SUBPIX_CAP, blendL );
#endif
#if FXAA_SUBPIX == 2
  float blendL = rangeL / range;
#endif
#if FXAA_DEBUG_PASSTHROUGH
#if FXAA_SUBPIX == 0
  float blendL = 0.0;
#endif
  // NOTE( N8 ): red = no blend, yellow = much blend ?
  return vec3( 1.0, blendL / FXAA_SUBPIX_CAP, 0.0 );
#endif



  //////////////////////////////////////////
  // CHOOSE VERTICAL OR HORIZONTAL SEARCH //
  //////////////////////////////////////////
  {
    // FXAA uses the following local neighborhood,
    //
    //     NW N NE
    //     W  M  E
    //     SW S SE
    //
    // To compute an edge amount for both vertical and horizontal directions.
    // Note edge detect filters like Sobel fail on single pixel lines through M.
    // FXAA takes the weighted average magnitude of the high-pass values
    // for rows and columns as an indication of local edge amount.
    //
    // A lowpass value for anti-sub-pixel-aliasing is computed as
    //     (N+W+E+S+M+NW+NE+SW+SE)/9.
    // This full box pattern has higher quality than other options.
    //
    // Note following this block, both vertical and horizontal cases
    // flow in parallel (reusing the horizontal variables).
  }
  vec3 rgbNW = textureOffset( tex, pos, ivec2( -1, -1 ) ).xyz;
  vec3 rgbNE = textureOffset( tex, pos, ivec2(  1, -1 ) ).xyz;
  vec3 rgbSW = textureOffset( tex, pos, ivec2( -1,  1 ) ).xyz;
  vec3 rgbSE = textureOffset( tex, pos, ivec2(  1,  1 ) ).xyz;
#if( FXAA_SUBPIX_FASTER == 0 ) && ( FXAA_SUBPIX > 0 )
  rgbL += ( rgbNW + rgbNE + rgbSW + rgbSE );
  rgbL *= vec3( 1.0 / 9.0 );
#endif
  float lumaNW = Luma( rgbNW );
  float lumaNE = Luma( rgbNE );
  float lumaSW = Luma( rgbSW );
  float lumaSE = Luma( rgbSE );
  float edgeVert =
    abs( ( 0.25 * lumaNW ) + ( -0.5 * lumaN ) + ( 0.25 * lumaNE ) ) +
    abs( ( 0.50 * lumaW  ) + ( -1.0 * lumaM ) + ( 0.50 * lumaE  ) ) +
    abs( ( 0.25 * lumaSW ) + ( -0.5 * lumaS ) + ( 0.25 * lumaSE ) );
  float edgeHorz =
    abs( ( 0.25 * lumaNW ) + ( -0.5 * lumaW ) + ( 0.25 * lumaSW ) ) +
    abs( ( 0.50 * lumaN  ) + ( -1.0 * lumaM ) + ( 0.50 * lumaS  ) ) +
    abs( ( 0.25 * lumaNE ) + ( -0.5 * lumaE ) + ( 0.25 * lumaSE ) );
  bool horzSpan = edgeHorz >= edgeVert;
#if FXAA_DEBUG_HORZVERT
  if( horzSpan ) return vec3( 1.0, 0.75, 0.0 );
  else return vec3( 0.0, 0.50, 1.0 );
#endif
  float lengthSign = horzSpan ? -rcpFrame.y : -rcpFrame.x;
  if( !horzSpan )
  {
    lumaN = lumaW;
    lumaS = lumaE;
  }
  float gradientN = abs( lumaN - lumaM );
  float gradientS = abs( lumaS - lumaM );
  lumaN = ( lumaN + lumaM ) * 0.5;
  lumaS = ( lumaS + lumaM ) * 0.5;

  ////////////////////////////////////////////////////
  // CHOOSE SIDE OF PIXEL WHERE GRADIENT IS HIGHEST //
  ////////////////////////////////////////////////////
  {
    // This chooses a pixel pair.
    // For "horzSpan == true" this will be a vertical pair,
    //
    //     [N]     N
    //     [M] or [M]
    //      S     [S]
    //
    // Note following this block, both {N,M} and {S,M} cases
    // flow in parallel (reusing the {N,M} variables).
    //
    // This pair of image rows or columns is searched below
    // in the positive and negative direction
    // until edge status changes
    // ( or the maximum number of search steps is reached).
  }
  bool pairN = gradientN >= gradientS;
#if FXAA_DEBUG_PAIR
  if( pairN ) return vec3( 0.0, 0.0, 1.0 );
  else return vec3( 0.0, 1.0, 0.0 );
#endif
  if( !pairN )
  {
    lumaN = lumaS;
    gradientN = gradientS;
    lengthSign *= -1.0;
  }
  vec2 posN;
  posN.x = pos.x + ( horzSpan ? 0.0 : lengthSign * 0.5 );
  posN.y = pos.y + ( horzSpan ? lengthSign * 0.5 : 0.0 );

  ///////////////////////////////////
  // CHOOSE SEARCH LIMITING VALUES //
  ///////////////////////////////////
  {
    // Search limit (+/- gradientN) is a function of local gradient.
  }
  gradientN *= FXAA_SEARCH_THRESHOLD;

  ////////////////////////////////////////////////////////////////////////////
  // SEARCH IN BOTH DIRECTIONS UNTIL FIND LUMA PAIR AVERAGE IS OUT OF RANGE //
  ////////////////////////////////////////////////////////////////////////////
  {
    // This loop searches either in vertical or horizontal directions,
    // and in both the negative and positive direction in parallel.
    // This loop fusion is faster than searching separately.
    //
    // The search is accelerated using FXAA_SEARCH_ACCELERATION length box
    // filter via anisotropic filtering with specified texture gradients.
  }
  vec2 posP = posN;
  vec2 offNP = horzSpan ?
    vec2( rcpFrame.x, 0.0 ) :
    vec2( 0.0f, rcpFrame.y );
  float lumaEndN = lumaN;
  float lumaEndP = lumaN;
  bool doneN = false;
  bool doneP = false;
#if FXAA_SEARCH_ACCELERATION == 1
  posN += offNP * vec2( -1.0, -1.0 );
  posP += offNP * vec2(  1.0,  1.0 );
#endif
#if FXAA_SEARCH_ACCELERATION == 2
  posN += offNP * vec2( -1.5, -1.5 );
  posP += offNP * vec2(  1.5,  1.5 );
  offNP *= vec2( 2.0, 2.0 );
#endif
#if FXAA_SEARCH_ACCELERATION == 3
  posN += offNP * vec2( -2.0, -2.0 );
  posP += offNP * vec2(  2.0,  2.0 );
  offNP *= vec2( 3.0, 3.0 );
#endif
#if FXAA_SEARCH_ACCELERATION == 4
  posN += offNP * vec2( -2.5, -2.5 );
  posP += offNP * vec2(  2.5,  2.5 );
  offNP *= vec2( 4.0, 4.0 );
#endif
  for( int i = 0; i < FXAA_SEARCH_STEPS; i++ )
  {
#if FXAA_SEARCH_ACCELERATION == 1
    if( !doneN ) lumaEndN = Luma( texture( tex, posN.xy ).xyz );
    if( !doneP ) lumaEndP = Luma( texture( tex, posP.xy ).xyz );
#else
    if( !doneN )
      lumaEndN = Luma( textureGrad( tex, posN.xy, offNP, offNP ).xyz );
    if( !doneP )
      lumaEndP = Luma( textureGrad( tex, posP.xy, offNP, offNP ).xyz );
#endif
    doneN = doneN || ( abs( lumaEndN - lumaN ) >= gradientN );
    doneP = doneP || ( abs( lumaEndP - lumaN ) >= gradientN );
    if( doneN && doneP ) break;
    if( !doneN ) posN -= offNP;
    if( !doneP ) posP += offNP;
  }

  //////////////////////////////////////////////////////
  // HANDLE IF CENTER IS ON POSITIVE OR NEGATIVE SIDE //
  //////////////////////////////////////////////////////
  {
    // FXAA uses the pixel's position in the span
    // in combination with the values (lumaEnd*) at the ends of the span,
    // to determine filtering.
    //
    // This step computes which side of the span the pixel is on.
    // On negative side if dstN < dstP,
    //
    //      posN        pos                      posP
    //       |-----------|------|------------------|
    //       |           |      |                  |
    //       |<--dstN--->|<---------dstP---------->|
    //                          |
    //                     span center
  }
  float dstN = horzSpan ? pos.x - posN.x : pos.y - posN.y;
  float dstP = horzSpan ? posP.x - pos.x : posP.y - pos.y;
  bool directionN = dstN < dstP;
#if FXAA_DEBUG_NEGPOS
  if( directionN ) return vec3( 1.0, 0.0, 0.0 );
  else return vec3( 0.0, 0.0, 1.0 );
#endif
  lumaEndN = directionN ? lumaEndN : lumaEndP;

  //////////////////////////////////////////////////////////////////
  // CHECK IF PIXEL IS IN SECTION OF SPAN WHICH GETS NO FILTERING //
  //////////////////////////////////////////////////////////////////
  {
    // If both the pair luma at the end of the span (lumaEndN)
    // and middle pixel luma (lumaM)
    // are on the same side of the middle pair average luma (lumaN),
    // then don't filter.
    //
    // Cases,
    //
    // ( 1.) "L",
    //
    //                lumaM
    //                  |
    //                  V    XXXXXXXX <- other line averaged
    //          XXXXXXX[X]XXXXXXXXXXX <- source pixel line
    //         |      .      |
    //     --------------------------
    //        [ ]xxxxxx[x]xx[X]XXXXXX <- pair average
    //     --------------------------
    //         ^      ^ ^    ^
    //         |      | |    |
    //         .      |<---->|<---------- no filter region
    //         .      | |    |
    //         . center |    |
    //         .        |  lumaEndN
    //         .        |    .
    //         .      lumaN  .
    //         .             .
    //         |<--- span -->|
    //
    //
    // (2.) "^" and "-",
    //
    //                                <- other line averaged
    //           XXXXX[X]XXX          <- source pixel line
    //          |     |     |
    //     --------------------------
    //         [ ]xxxx[x]xx[ ]        <- pair average
    //     --------------------------
    //          |     |     |
    //          |<--->|<--->|<---------- filter both sides
    //
    //
    // (3.) "v" and inverse of "-",
    //
    //     XXXXXX           XXXXXXXXX <- other line averaged
    //     XXXXXXXXXXX[X]XXXXXXXXXXXX <- source pixel line
    //          |     |     |
    //     --------------------------
    //     XXXX[X]xxxx[x]xx[X]XXXXXXX <- pair average
    //     --------------------------
    //          |     |     |
    //          |<--->|<--->|<---------- don't filter both!
    //
    //
    // Note the "v" case for FXAA requires no filtering.
    // This is because the inverse of the "-" case is the "v".
    // Filtering "v" case turns open spans like this,
    //
    //     XXXXXXXXX
    //
    // Into this (which is not desired),
    //
    //     x+.   .+x
    //     XXXXXXXXX
    //
  }
  if( ( ( lumaM - lumaN ) < 0.0 ) == ( ( lumaEndN - lumaN ) < 0.0 ) )
    lengthSign = 0.0;


  //////////////////////////////////////////////
  // COMPUTE SUB-PIXEL OFFSET AND FILTER SPAN //
  //////////////////////////////////////////////
  {
    // FXAA filters using a bilinear texture fetch offset
    // from the middle pixel M towards the center of the pair (NM below).
    // Maximum filtering will be half way between pair.
    // Reminder, at this point in the code,
    // the {N,M} pair is also reused for all cases: {S,M}, {W,M}, and {E,M}.
    //
    //     +-------+
    //     |       |    0.5 offset
    //     |   N   |     |
    //     |       |     V
    //     +-------+....---
    //     |       |
    //     |   M...|....---
    //     |       |     ^
    //     +-------+     |
    //     .       .    0.0 offset
    //     .   S   .
    //     .       .
    //     .........
    //
    // Position on span is used to compute sub-pixel filter offset using
    // simple ramp,
    //
    //              posN           posP
    //               |\             |<------- 0.5 pixel offset into pair pixel
    //               | \            |
    //               |  \           |
    //     ---.......|...\..........|<------- 0.25 pixel offset into pair pixel
    //      ^        |   ^\         |
    //      |        |   | \        |
    //      V        |   |  \       |
    //     ---.......|===|==========|<------- 0.0 pixel offset (ie M pixel)
    //      ^        .   |   ^      .
    //      |        .  pos  |      .
    //      |        .   .   |      .
    //      |        .   . center   .
    //      |        .   .          .
    //      |        |<->|<---------.-------- dstN
    //      |        .   .          .
    //      |        .   |<-------->|<------- dstP
    //      |        .             .
    //      |        |<------------>|<------- spanLength
    //      |
    //     subPixelOffset
    //
  }
  float spanLength = ( dstP + dstN );
  dstN = directionN ? dstN : dstP;
  float subPixelOffset = ( 0.5 + ( dstN * ( -1.0 / spanLength ) ) ) * lengthSign;
#if FXAA_DEBUG_OFFSET
  float ox = horzSpan ? 0.0 : subPixelOffset * 2.0 / rcpFrame.x;
  float oy = horzSpan ? subPixelOffset * 2.0 / rcpFrame.y : 0.0;
  if( ox < 0.0 ) return mix( vec3( lumaO ), vec3( 1.0, 0.0, 0.0 ), -ox);
  if( ox > 0.0 ) return mix( vec3( lumaO ), vec3( 0.0, 0.0, 1.0 ),  ox);
  if( oy < 0.0 ) return mix( vec3( lumaO ), vec3( 1.0, 0.6, 0.2 ), -oy);
  if( oy > 0.0 ) return mix( vec3( lumaO ), vec3( 0.2, 0.6, 1.0 ),  oy);
  return vec3( lumaO, lumaO, lumaO );
#endif
  vec3 rgbF = texture( tex, vec2(
        pos.x + ( horzSpan ? 0.0 : subPixelOffset ),
        pos.y + ( horzSpan ? subPixelOffset : 0.0 ) ) ).xyz;
#if FXAA_SUBPIX == 0
  return rgbF;
#else
  return mix( rgbL, rgbF, blendL );
#endif
}


void main()
{
  // i think this = linear color ^ ( 1.0 / 2.2 )
  // vec3 sRGBColor = texture( quadTexture, uvs ).xyz;
  // outColor = vec4( sRGBColor, 1.0 );

#if APPLY_FXAA
  outColor = vec4(
      FxaaPixelShader( uvs, quadTexture, rcpFrame ),
      1.0 );
#else
  outColor = vec4( texture( quadTexture, uvs ).xyz, 1.0 );
#endif
}
