#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>


#define MAXOBJS 6

#define	OTSIZE	(4096)

typedef struct {
	DRAWENV	draw;		/* drawing environment */
	DISPENV	disp;		/* display environment */
	u_long	ot[OTSIZE];	/* Ordering Table */
	POLY_F4	polys[MAXOBJS];	/* polys */
} DB;

/* double buffer */
DB	db[2];		

/* current double buffer */
DB	*cdb;	

/*************************************************************/
// Hardcoded vertices + normals for a cube
/*************************************************************/

#define SCR_Z (512)	/* screen depth */
#define CUBESIZE 50

// Vertices

static SVECTOR P0 = {-CUBESIZE/2,-CUBESIZE/2,-CUBESIZE/2,0};
static SVECTOR P1 = { CUBESIZE/2,-CUBESIZE/2,-CUBESIZE/2,0};
static SVECTOR P2 = { CUBESIZE/2, CUBESIZE/2,-CUBESIZE/2,0};
static SVECTOR P3 = {-CUBESIZE/2, CUBESIZE/2,-CUBESIZE/2,0};

static SVECTOR P4 = {-CUBESIZE/2,-CUBESIZE/2, CUBESIZE/2,0};
static SVECTOR P5 = { CUBESIZE/2,-CUBESIZE/2, CUBESIZE/2,0};
static SVECTOR P6 = { CUBESIZE/2, CUBESIZE/2, CUBESIZE/2,0};
static SVECTOR P7 = {-CUBESIZE/2, CUBESIZE/2, CUBESIZE/2,0};

static SVECTOR	*vertices[6*4] = {
	&P0,&P1,&P2,&P3,
	&P1,&P5,&P6,&P2,
	&P5,&P4,&P7,&P6,
	&P4,&P0,&P3,&P7,
	&P4,&P5,&P1,&P0,
	&P6,&P7,&P3,&P2,
};

// Normals

static SVECTOR N0 = { ONE,   0,    0, 0,};
static SVECTOR N1 = {-ONE,   0,    0, 0,};
static SVECTOR N2 = {0,    ONE,    0, 0,};
static SVECTOR N3 = {0,   -ONE,    0, 0,};
static SVECTOR N4 = {0,      0,  ONE, 0,};
static SVECTOR N5 = {0,      0, -ONE, 0,};

static SVECTOR	*normals[6] = {	&N5, &N0, &N4, &N1, &N3, &N2};

/*************************************************************/
// Light stuff
/*************************************************************/

static SVECTOR lightDirVec = {0,0,-4096}; // Directly into screen

/* Local Light Matrix */
static MATRIX	llm;

/* Local Color Matrix */
static MATRIX	lcm = {4096,0,0, 4096,0,0, 4096,0,0, 0,0,0};


/*************************************************************/
// Prototypes
/*************************************************************/

void initPrimitives(DB *buffer);

void addCube(u_long *ot, POLY_F4 *s,VECTOR *posVec,SVECTOR *rotVecs);

/*************************************************************/

int main()
{
	SVECTOR	rotAngVec  = { 0, 0, 0};
	VECTOR	posVec  = {0, 0, 2*SCR_Z};
	
	ResetGraph(0);
	ResetCallback();
	SetGraphDebug(0);
	
	InitGeom();
	SetGeomOffset(160, 120);	
	SetGeomScreen(SCR_Z);	
	SetVideoMode(MODE_PAL);
	SetDispMask(1);		

	/* Render upper part of framebuffer, while drawing to bottom, and vice versa */

	SetDefDrawEnv(&db[0].draw, 0,   0, 320, 240);
	SetDefDrawEnv(&db[1].draw, 0, 240, 320, 240);
	SetDefDispEnv(&db[0].disp, 0, 240, 320, 240);
	SetDefDispEnv(&db[1].disp, 0,   0, 320, 240);
	
	SetBackColor(100,100,100);
	
	initPrimitives(&db[0]);
	initPrimitives(&db[1]);
	
	
	while (1) 
	{
		cdb  = (cdb==db)? db+1: db;	// Switch double buffer
		
		ClearOTagR(cdb->ot, OTSIZE);	
		
		rotAngVec.vz -= 5;
		rotAngVec.vy += 15;
		rotAngVec.vx += 8;
		
		// transpose + light + perspective + add to OT
		addCube(cdb->ot, cdb->polys, &posVec,&rotAngVec);
		
		
		DrawSync(0);		
		VSync(0);

		/* swap double buffer */
		PutDispEnv(&cdb->disp);
		PutDrawEnv(&cdb->draw); 
		
		/* draw OT */
		//DrawOTag(cdb->ot);	
		DrawOTag(cdb->ot+OTSIZE-1);	
	
	}
	
}

// ***************************************************
//	initPrimitives
//
//	Initializes all the POLY_F4 primitive structs
// ***************************************************

void initPrimitives(DB *buffer)
{
		
	int i = 0;
	POLY_F4 *poly;
	
	buffer->draw.isbg = 1;
	setRGB0(&buffer->draw, 60, 120, 120);
	
	// Initialize all primitive structs
	
	for(poly = buffer->polys;i<MAXOBJS;i++,poly++)
	{
		SetPolyF4(poly);
	}
	
}

// ***************************************************
//
//  addCube
//
//  takes a POLY_F4 primitive and does:
//  - coordinate and perspective transform
//  - light calculation
//  - add to OT
//
// ***************************************************

void addCube(u_long *ot, POLY_F4 *s,VECTOR *posVec,SVECTOR *rotVecs)
{
	int	i;
	long	p, otz, opz, flg;
	int	isomote;
	SVECTOR	**vp, **np;
	CVECTOR colorIn;
	CVECTOR colorOut;
	SVECTOR normal;
	MATRIX rottrans;
	
	MATRIX	inverseLightMatrix;
	SVECTOR inverseLightVector;
	
	// When we call the RotAverageNclip4 all the surfaces are rotated using the rotation matrix set with RotMatrix.
	// The problem is, the precalculated normals we pass to the light calculation function are static, and does therefore not rotate with the cube.
	// So when we set the colors returned from the light calculation function NormalColorCol back onto the surfaces, it appears as if the light is
	// following the cubes rotation.
	//
	// To counter this, we can do 1 of two things:
	// A) recalculate the rotated normals of every surface on every frame
	// B) just counter-rotate the light itself
	// 
	// The logical thing is to do B: counter rotate the light, by setting the negated cube rotation into the light matrix.
	// So if we rotate the cube 10 degree, we rotate the light -10 degree, and then the light will appear to be static in world space as we want.
	
	// Create a rotation matrix for the light:

	inverseLightVector.vx = -rotVecs->vx;
	inverseLightVector.vy = -rotVecs->vy;
	inverseLightVector.vz = -rotVecs->vz;
	
	// To see the problem we get from not couter-rotating the light, uncomment the 3 lines below:
	/*
	inverseLightVector.vx = 0;
	inverseLightVector.vy = 0;
	inverseLightVector.vz = 0;
	*/
	
	RotMatrixZYX(&inverseLightVector,&inverseLightMatrix); // The reverse axis sequence of the normal RotMatrix() function, to counter the rotation
	ApplyMatrixSV(&inverseLightMatrix,(SVECTOR*)&lightDirVec,(SVECTOR*)&llm);
	
	SetColorMatrix(&lcm);
	SetLightMatrix(&llm);
	
	// Create a rotation+transpose matrix for the cube
	
	RotMatrix_gte(rotVecs, &rottrans);
	TransMatrix(&rottrans,posVec);
	
	//PushMatrix();
	
	SetRotMatrix(&rottrans);		
	SetTransMatrix(&rottrans);	
	

	vp = vertices;		/* vp: vertex pointer (work) */
	np = normals;		/* np: normal pointer (work) */
	
	// Loop through all 6 surfaces and render them

	for (i = 0; i < 6; i++, s++, vp += 4, np++) {

	SetRotMatrix(&rottrans);		
	
		isomote = RotAverageNclip4(vp[0], vp[1], vp[2], vp[3], 
			(long *)&s->x0, (long *)&s->x1, 
			(long *)&s->x3, (long *)&s->x2, &p, &otz, &flg);
			
		if (isomote <= 0) continue;	

		/* Put into OT:
		 * The length of the OT is assumed to 4096. */
		if (otz > 0 && otz < 4096) {	
			// just use a hardcoded unlit color for the surface
			colorIn.r = 200;
			colorIn.g = 200;
			colorIn.b = 0;
			// Get local light value..
			NormalColorCol(*np, &colorIn, &colorOut);
			// and apply it to surface
			s->r0 = colorOut.r;
			s->g0 = colorOut.g;
			s->b0 = colorOut.b;
			
			AddPrim(ot+otz, s); 
		}
	}
	
	//PopMatrix();		
}