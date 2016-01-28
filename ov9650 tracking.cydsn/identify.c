#include"identify.h"
#include<array>
#include<vector>
#include<queue>
#include<bitset>
#include<algorithm>
#include<iterator>
#include<cmath>

template<class T> struct int_it: public std::iterator<std::input_iterator_tag,T,T,T,T>
{
	T n;
	int_it(T n): n(n) {}
	bool operator!=(const int_it &other) {return n!=other.n;}
	T& operator*() {return n;}
	int_it& operator++() {++n; return *this;}
};

#define MAX_BLOBS 256 //maximum number of blobs to process
#define SCALE_Y 1 //scale y^2 by this in distance calculation
#define SMALL_BLOB_FILTER 0.2 //remove blobs that are smaller than this fraction of the biggest blob in the component
//#define ELLIPSE //fit ellipses instead of circles
#define MIN_COMPONENT_SIZE 4 //minimum number of LEDs in component to attempt identification
#define EBUG_LEDS 6 //number of LEDs on each eBug

static const std::array<std::array<uint8_t,EBUG_LEDS>,10> seqs{{
    {{0,0,1,0,0,2}},
    {{0,0,3,0,1,3}},
    {{0,1,1,0,1,2}},
    {{0,2,1,0,2,2}},
    {{0,2,3,0,3,3}},
    {{0,3,1,0,3,2}},
    {{1,1,2,1,1,3}},
    {{1,2,2,1,2,3}},
    {{1,3,2,1,3,3}},
    {{2,2,3,2,3,3}},
    }};

static constexpr float pi=4*atanf(1);
static float min_rounding(std::vector<float> &angles) //minimise rounding when selecting evenly placed points on circle
{
    float angle=0,min=INFINITY;
    for(auto j:angles)
    {
        float r=0;
        for(auto k:angles)
        {
            float x=(k-j)*EBUG_LEDS/(2*pi)+EBUG_LEDS;
            x-=roundf(x);
            r+=x*x;
        }
        if(r<min)
        {
            min=r;
            angle=j;
        }
    }
    return angle;
}

static uint8 current_ebug;
void identify(std::vector<uint16> &leds,point *points)
{
	using namespace std;
	
	#ifdef ELLIPSE
		//TODO fitEllipse, etc.
	#else
		uint16 x,y;
		float radius;
		if(leds.size()==3) //special case: find unique circle through 3 points
		{
			int32 ax=points[leds[0]].x,ay=points[leds[0]].y;
			int32 bx=points[leds[1]].x,by=points[leds[1]].y;
			int32 cx=points[leds[2]].x,cy=points[leds[2]].y;
			int32 d=2*(ax*(by-cy)+bx*(cy-ay)+cx*(ay-by));
			x=((ax*ax+ay*ay)*(by-cy)+(bx*bx+by*by)*(cy-ay)+(cx*cx+cy*cy)*(ay-by))/d;
			y=((ax*ax+ay*ay)*(cx-bx)+(bx*bx+by*by)*(ax-cx)+(cx*cx+cy*cy)*(bx-ax))/d;
			radius=sqrtf((ax-x)*(ax-x)+(ay-y)*(ay-y));
		}
		else
		{
			uint16 a=-1,b=-1;
			uint32 max_dist2=0;
			for(auto i=leds.begin();i!=leds.end();i++) for(auto j=leds.begin();j!=i;j++)
			{
				int32 x=points[*i].x-points[*j].x;
				int32 y=points[*i].y-points[*j].y;
				uint32 dist2=x*x+y*y;
				if(dist2>max_dist2) max_dist2=dist2,a=*i,b=*j;
			}
			x=(points[a].x+points[b].x)/2;
			y=(points[a].y+points[b].y)/2;
			radius=sqrtf((points[a].x-x)*(points[a].x-x)+(points[a].y-y)*(points[a].y-y));
		}
		
		vector<float> angles;
		angles.reserve(leds.size());
        for(auto i:leds) angles.push_back(atan2f(points[i].y-y,points[i].x-x));
        float base_angle=min_rounding(angles);
        
        array<uint8,EBUG_LEDS> rounded;
        fill(rounded.begin(),rounded.end(),4); //4 means unmatched
        array<uint16,EBUG_LEDS> size{};
		for(uint16 i=0;i!=leds.size();i++) //round each led angle to nearest discrete point
        {
			point &p=points[leds[i]];
            uint8 z=roundf((angles[i]-base_angle)*EBUG_LEDS/(2*pi)+EBUG_LEDS);
            z%=EBUG_LEDS;
            if(size[z]<p.size) size[z]=p.size,rounded[z]=p.colour;
        }
		
		bool matched=false;
        pair<uint8,uint8> id;
        for(uint8 j=0;j<seqs.size();j++) for(uint8 k=0;k<EBUG_LEDS;k++) //try to match observed sequence to one from the table
        {
            uint8 l;
            for(l=0;l<EBUG_LEDS;l++) if(rounded[l]!=4 and rounded[l]!=seqs[j][(k+l)%EBUG_LEDS]) break;
            if(l==EBUG_LEDS)
            {
				matched^=true;
                if(not matched) break; //ensure there is only one match
                id=make_pair(j,k);
            }
        }
		if(matched)
		{
			float angle=(EBUG_LEDS-0.5f-id.second)*(2*pi)/EBUG_LEDS+base_angle;
			angle*=65536.f/(2*pi);
			eBugs[current_ebug++]={id.first,x,y,LO16(int32(angle)),uint16(65536.f/radius)};
		}
	#endif
}

uint8 identify(point *points,uint16 n)
{
	using namespace std;
	
	array<vector<uint16>,MAX_BLOBS> knngraph;
	bitset<MAX_BLOBS> done{}; //TODO use custom class with bitbanding (and always allocate on stack)
	
	if(n>MAX_BLOBS) n=MAX_BLOBS;
    for(uint16 i=0;i<n;i++) //determine all edges in 2-nearest neighbour graph
    {
        array<uint16,3> neighbours;
        array<int32,MAX_BLOBS> dists;
        for(uint16 j=0;j<n;j++)
        {
            int32 x=points[j].x-points[i].x;
			int32 y=points[j].y-points[i].y;
            dists[j]=x*x+y*y*SCALE_Y;
        }
		
		uint32 too_small=points[i].size*SMALL_BLOB_FILTER;
		uint32 too_big=points[i].size/SMALL_BLOB_FILTER;
		
		partial_sort_copy(int_it<uint16>(0),int_it<uint16>(n),neighbours.begin(),neighbours.end(), //sort distances
            [&dists](uint16 a,uint16 b)
			{
				return dists[a]<dists[b];
			});
		if(points[neighbours[1]].size>too_big or points[neighbours[2]].size>too_big)
		{
			done[i]=true;
			continue;
		}
		
		bitset<MAX_BLOBS> remove;
		for(uint16 j=0;j<n;j++) remove[j]=points[j].size<too_small or points[j].size>too_big;
		partial_sort_copy(int_it<uint16>(0),int_it<uint16>(n),neighbours.begin(),neighbours.end(), //sort distances
            [&remove,&dists](uint16 a,uint16 b)
			{
				if(remove[a]) return false;
				else if(remove[b]) return true;
				else return dists[a]<dists[b];
			});

        for(uint8 j=1;j<neighbours.size();j++) if(not remove[j])
		{
			knngraph[i].push_back(neighbours[j]);
			knngraph[neighbours[j]].push_back(i);
		}
    }

	vector<uint16> component;
    queue<uint16> q;
	current_ebug=0;
    for(uint16 i=0;i<n;i++) //partition graph into connected components
    {
        if(done[i]) continue;
        component.clear();
		q.push(i);
        while(not q.empty()) //breadth-first spanning tree search
        {
            auto v=q.front();
            q.pop();

            if(not done[v]) done[v]=true,component.push_back(v);
            for(auto j:knngraph[v])
                if(not done[j]) q.push(j);
        }
        if(component.size()>=MIN_COMPONENT_SIZE) identify(component,points);
	}
	
	return current_ebug;
}