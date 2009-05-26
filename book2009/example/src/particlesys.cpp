#include <iostream>
#include <list>
#include <cstdlib>
#include <cmath>
#include <iterator>
#include <fstream>

const bool INFECTED = true;
const int  MAX_SPEED = 50; // walking speed: 50 [m/min]
const double dt = 1;       // 1 min
const double pi = 3.141592654;
const int INFECT_PROBABILITY = 50; //50%
const int DELITESCENCE = 3*24*60; //3 days

struct area{
public:
  area(int w, int h):x0(w/2), y0(h/2), width(w), height(h){
    cells=new char[w*h];
    reset();
  }
  area(int x, int y, int w, int h):x0(x), y0(y), width(w), height(h){ }

  virtual ~area(){ delete cells; }

  char* at(int x, int y){
    return cells+y*width+x;
  }

  void reset(){ memset(cells, 0, width*height); }

  template<class Loc>
  bool contains(const Loc& l){
    return abs(l.x-x0)<=width/2 && abs(l.y-y0)<=height/2;
  }

  int x0;
  int y0;
  int width;
  int height;

private:
  char* cells;
};

struct physics{
  physics(){}
  physics(int _x, int _y, int _s, int _d):x(_x), y(_y), speed(_s), direction(_d){}
  physics(const physics& p):x(p.x), y(p.y), speed(p.speed), direction(p.direction){}
  physics& operator=(const physics& p){
    x=p.x; y=p.y;
    speed=p.speed; direction = p.direction;
  }

  bool operator==(const physics& p){
    return x==p.x && y==p.y && speed== p.speed && direction == p.direction;
  }

  void move(double dt){
    x+=static_cast<int>(static_cast<double>(speed)*dt*cos(static_cast<double>(direction)/180.0*pi));
    y+=static_cast<int>(static_cast<double>(speed)*dt*sin(static_cast<double>(direction)/180.0*pi));
    speed = rand() % MAX_SPEED;
  }

  int x;
  int y;
  int speed;
  int direction;
};

class person{
public:
  person(bool x=false):_infected(x), _delitescence(0){}
  person(const person& p):_infected(p._infected), _delitescence(p._delitescence),_loc(p._loc){}
  person& operator=(const person& p){
    _infected = p._infected;
    _delitescence = p._delitescence;
    _loc = p._loc;
    return *this;
  }

  physics location() const{ return _loc; }
  void set_location(const physics& l){ _loc=l; }
  bool infected() const { return _infected; }
  void set_infected(bool x) { _infected = x; }
  bool ill() const { return _delitescence > DELITESCENCE; }
  void inc_delitescence() { ++_delitescence; }

  void move_inside(area& a, double dt){
    _loc.move(dt);
    if(_loc.x<0 || _loc.x>a.width)
      _loc.direction = (180 - _loc.direction + 360) % 360;
    if(_loc.y<0 || _loc.y>a.height)
      _loc.direction = (-_loc.direction + 360) % 360;
    if(_loc.x<0)
      _loc.x=-_loc.x;
    if(_loc.x>a.width)
      _loc.x=2*a.width-_loc.x;
    if(_loc.y<0)
      _loc.y=-_loc.y;
    if(_loc.y>a.height)
      _loc.y=2*a.height-_loc.y;
  }

  void move_to(area& a){
    if(a.x0 == _loc.x)
      _loc.direction = a.y0 > _loc.y ? 90 : 180;
    else
      _loc.direction = static_cast<int>(atan(static_cast<double>(a.y0-_loc.y)/
                                             static_cast<double>(a.x0-_loc.x))/pi*180.0);
  }

private:
  bool _infected;
  int  _delitescence;
  physics _loc;
};

class scheduler{
public:
  static scheduler& inst(){
    static scheduler _inst;
    return _inst;
  }

  void setup(int w, int h, int n){
    a=new area(w, h);
    for(int i=0; i<n-1; ++i)
      people.push_back(new person());
    people.push_back(new person(INFECTED));
    put_people(people, *a);
    n_infected = 1;
  }

  void set_hospital(int x, int y, int w, int h){
    hospital=new area(x, y, w, h);
  }

  void run(){
    for(int tm=0; n_infected < people.size()*90/100; tm++){
      a->reset();
      move();
      infect();
      std::cout<<"time "<<tm/60<<":"<<tm%60<<" "
               <<n_infected<<"/"<<people.size()<<" are infected\r";
      diagram.push_back(n_infected);
    }
    write_diagram();
  }

private:
  scheduler(){}
  ~scheduler(){ 
    delete a;
    delete hospital;
    for(Population::iterator it=people.begin(); it!=people.end(); ++it)
      delete *it;
  }

  void move(){
    for(Population::iterator it=people.begin(); it!=people.end(); ++it){
      if((*it)->ill()){
        if(hospital->contains((*it)->location()))
          (*it)->move_inside(*hospital, dt);
        else
          (*it)->move_to(*hospital);
      }
      else
        (*it)->move_inside(*a, dt);
      if((*it)->infected()){
        *(a->at((*it)->location().x, (*it)->location().y))=1;
        (*it)->inc_delitescence();
      }
    }
  }

  void infect(){
    for(Population::iterator it=people.begin(); it!=people.end(); ++it)
      if(!(*it)->infected() &&
         *(a->at((*it)->location().x, (*it)->location().y))&&
         rand()%100 > INFECT_PROBABILITY){
        n_infected++;
        (*it)->set_infected(true);
      }
  }

  template<class Coll>
  void put_people(Coll& coll, area& a){
    for(typename Coll::iterator it=coll.begin(); it!=coll.end(); ++it){
      (*it)->set_location(physics(rand()%a.width, rand()%a.height,
                                  rand()%MAX_SPEED, rand()%360));
    }
  }

  void write_diagram(){
    std::ofstream file("diagram.csv");
    std::copy(diagram.begin(), diagram.end(), std::ostream_iterator<int>(file, "\n"));
  }

  area* a;
  area* hospital;
  typedef std::list<person*> Population;
  Population people;
  int n_infected;
  std::list<int> diagram;
};

int main(int argc, char** argv){
  //Beijing has people density as 888 persons/km^2, ==> 1061 m^2 is the best fit
  scheduler::inst().setup(1061, 1061, 1000);
  scheduler::inst().set_hospital(500, 500, 50, 50);
  scheduler::inst().run();
}