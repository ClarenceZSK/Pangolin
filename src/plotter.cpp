/* This file is part of the Pangolin Project.
 * http://github.com/stevenlovegrove/Pangolin
 *
 * Copyright (c) 2011 Steven Lovegrove
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <pangolin/plotter.h>
#include <pangolin/display.h>
#include <pangolin/gldraw.h>
#include <pangolin/display_internal.h>

#include <limits>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <map>

using namespace std;

namespace pangolin
{

extern __thread PangolinGl* context;

#ifndef HAVE_GLES
static void* font = GLUT_BITMAP_HELVETICA_12;
#endif

const static int num_plot_colours = 12;
const static float plot_colours[][3] =
{
    {1.0, 0.0, 0.0},
    {0.0, 1.0, 0.0},
    {0.0, 0.0, 1.0},
    {1.0, 0.0, 1.0},
    {0.5, 0.5, 0.0},
    {0.5, 0.0, 0.0},
    {0.0, 0.5, 0.0},
    {0.0, 0.0, 0.5},
    {0.5, 0.0, 1.0},
    {0.0, 1.0, 0.5},
    {1.0, 0.0, 0.5},
    {0.0, 0.5, 1.0}
};


const static float colour_bg[3] = {0.0,0.0,0.0};
const static float colour_tk[3] = {0.1,0.1,0.1};
const static float colour_ms[3] = {0.3,0.3,0.3};
const static float colour_ax[3] = {0.5,0.5,0.5};

Plotter::Plotter(DataLog* log, float left, float right, float bottom, float top, float tickx, float ticky)
    : log(log), track_front(true), mouse_state(0),lineThickness(1.5f), draw_mode(0), plot_mode(TIME_SERIES)
{
    this->handler = this;
    int_x[0] = int_x_dflt[0] = left;
    int_x[1] = int_x_dflt[1] = right;
    int_y[0] = int_y_dflt[0] = bottom;
    int_y[1] = int_y_dflt[1] = top;
    vo[0] = vo[1] = 0;
    ticks[0] = tickx;
    ticks[1] = ticky;
    
    for( unsigned int i=0; i<show_n; ++i )
        show[i] = true;
}

void Plotter::DrawTicks()
{
    glColor3fv(colour_tk);
    glLineWidth(lineThickness);
    const int tx[2] = {
        (int)ceil(int_x[0] / ticks[0]),
        (int)ceil(int_x[1] / ticks[0])
    };
    
    const int ty[2] = {
        (int)ceil(int_y[0] / ticks[1]),
        (int)ceil(int_y[1] / ticks[1])
    };
    
    const int votx = ceil(vo[0]/ ticks[0]);
    const int voty = ceil(vo[1]/ ticks[1]);
    if( tx[1] - tx[0] < v.w/4 ) {
        for( int i=tx[0]; i<tx[1]; ++i ) {
            glDrawLine((i+votx)*ticks[0], int_y[0]+vo[1],   (i+votx)*ticks[0], int_y[1]+vo[1]);
        }
    }
    
    if( ty[1] - ty[0] < v.h/4 ) {
        for( int i=ty[0]; i<ty[1]; ++i ) {
            glDrawLine(int_x[0]+vo[0], (i+voty)*ticks[1],  int_x[1]+vo[0], (i+voty)*ticks[1]);
        }
    }
    
    glColor3fv(colour_ax);
    glDrawLine(0, int_y[0]+vo[1],  0, int_y[1]+vo[1] );
    glDrawLine(int_x[0]+vo[0],0,   int_x[1]+vo[0],0  );
    glLineWidth(1.0f);
}

void Plotter::DrawSequence(const DataSequence& seq)
{
#ifndef HAVE_GLES
    const int seqint_x[2] = {seq.IndexBegin(), seq.IndexEnd() };
    const int valid_int_x[2] = {
        std::max(seqint_x[0],(int)(int_x[0]+vo[0])),
        std::min(seqint_x[1],(int)(int_x[1]+vo[0]))
    };
    glLineWidth(lineThickness);
    
    glBegin(draw_modes[draw_mode]);
    for( int x=valid_int_x[0]; x<valid_int_x[1]; ++x )
        glVertex2f(x,seq[x]);
    glEnd();
    
    glLineWidth(1.0f);
#endif // HAVE_GLES
}

void Plotter::DrawSequenceHistogram(const DataLog& seq)
{
#ifndef HAVE_GLES
    size_t vec_size = std::min((unsigned)log->x, log->buffer_size);
    int idx_subtract = std::max(0,(int)(log->x)-(int)(log->buffer_size));
    vector<float> accum_vec(vec_size,0);
    
    for(int s=log->sequences.size()-1; s >=0; --s )
    {
        if( (s > 9) ||  show[s] )
        {
            const int seqint_x[2] = {seq.Sequence(s).IndexBegin(), seq.Sequence(s).IndexEnd() };
            const int valid_int_x[2] = {
                std::max(seqint_x[0],(int)(int_x[0]+vo[0])),
                std::min(seqint_x[1],(int)(int_x[1]+vo[0]))
            };
            
            glBegin(GL_TRIANGLE_STRIP);
            glColor3fv(plot_colours[s%num_plot_colours]);
            
            
            for( int x=valid_int_x[0]; x<valid_int_x[1]; ++x )
            {
                float val = seq.Sequence(s)[x];
                
                float & accum = accum_vec.at(x-idx_subtract);
                float before_val = accum;
                accum += val;
                glVertex2f(x-0.5,before_val);
                glVertex2f(x-0.5,accum);
                glVertex2f(x+0.5,before_val);
                glVertex2f(x+0.5,accum);
            }
            glEnd();
        }
    }
#endif // HAVE_GLES
}

void Plotter::DrawSequence(const DataSequence& x,const DataSequence& y)
{
#ifndef HAVE_GLES
    const unsigned minn = max(x.IndexBegin(),y.IndexBegin());
    const unsigned maxn = min(x.IndexEnd(),y.IndexEnd());
    
    glLineWidth(lineThickness);

    glBegin(draw_modes[draw_mode]);
    for( unsigned n=minn; n < maxn; ++n )
        glVertex2f(x[n],y[n]);
    glEnd();
    
    glLineWidth(1.0f);
#endif
}

void Plotter::Render()
{
#ifndef HAVE_GLES
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT);
#endif
    
    if( track_front )
    {
        vo[0] = log->x-int_x[1];
        //const float d = int_x[1] - log->x
        //int_x[0] -= d;
        //int_x[1] -= d;
    }
    
    glClearColor(0.0, 0.0, 0.0, 0.0);
    ActivateScissorAndClear();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(int_x[0]+vo[0], int_x[1]+vo[0], int_y[0]+vo[1], int_y[1]+vo[1], -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glDisable(GL_MULTISAMPLE);
    
    glLineWidth(1.5);
    glEnable(GL_LINE_SMOOTH);
    glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING);
    glDisable( GL_DEPTH_TEST );
    
    DrawTicks();
    
    if( log && log->sequences.size() > 0 ) {
        if( plot_mode==TIME_SERIES) {
            for( unsigned int s=0; s < log->sequences.size(); ++s ) {
                if( (s > 9) ||  show[s] ) {
                    glColor3fv(plot_colours[s%num_plot_colours]);
                    DrawSequence(log->Sequence(s));
                }
            }
        }else if( plot_mode==XY ) {
            for( unsigned int s=0; s < log->sequences.size() / 2; ++s ) {
                if( (s > 9) ||  show[s] ) {
                    glColor3fv(plot_colours[s%num_plot_colours]);
                    DrawSequence(log->Sequence(2*s),log->Sequence(2*s+1) );
                }
            }
        }else if( plot_mode==STACKED_HISTOGRAM ) {
            DrawSequenceHistogram(*log);
        }else {
            throw std::logic_error("Unknown plot mode.");
        }
    }
    
    if( mouse_state & MouseButtonLeft )
    {
        if( plot_mode==XY )
        {
            glColor3fv(colour_ms);
            glDrawLine(mouse_xy[0],int_y[0],  mouse_xy[0],int_y[1]);
            glDrawLine(int_x[0],mouse_xy[1],  int_x[1],mouse_xy[1]);
#ifdef HAVE_GLUT
            stringstream ss;
            ss << "(" << mouse_xy[0] << "," << mouse_xy[1] << ")";
            glColor3f(1.0,1.0,1.0);
            DisplayBase().ActivatePixelOrthographic();
            glRasterPos2f( v.l+5,v.b+5 );
            glutBitmapString(font,(unsigned char*)ss.str().c_str());
#endif
        }else{
            int xu = (int)mouse_xy[0];
            glColor3fv(colour_ms);
            glDrawLine(xu,int_y[0],  xu,int_y[1]);
#ifdef HAVE_GLUT
            stringstream ss;
            glColor3f(1.0,1.0,1.0);
            ss << "x=" << xu << " ";
            DisplayBase().ActivatePixelOrthographic();
            int tx = v.l+5;
            glRasterPos2f( tx,v.b+5 );
            glutBitmapString(font,(unsigned char*)ss.str().c_str());
            tx += glutBitmapLength(font,(unsigned char*)ss.str().c_str());
            for( unsigned int s=0; s<log->sequences.size(); ++s )
            {
                if( (s > show_n || show[s]) && log->Sequence(s).HasData(xu) )
                {
                    stringstream ss;
                    ss << " " << log->Sequence(s)[xu];
                    glColor3fv(plot_colours[s%num_plot_colours]);
                    glRasterPos2f( tx,v.b+5 );
                    glutBitmapString(font,(unsigned char*)ss.str().c_str());
                    tx += glutBitmapLength(font,(unsigned char*)ss.str().c_str());
                }
            }
#endif            
        }
    }
    
#ifdef HAVE_GLUT
    float ty = v.h+v.b-15;
    for (size_t i=0; i<log->labels.size(); ++i)
    {
        glColor3fv(plot_colours[i%num_plot_colours]);
        
        DisplayBase().ActivatePixelOrthographic();
        glRasterPos2f(v.l+5,ty);
        glutBitmapString(font,(unsigned char*)log->labels[i].c_str());
        ty -= 15;
    }
#endif
 
#ifndef HAVE_GLES
    glPopAttrib();
#endif
}

void Plotter::ResetView()
{
    track_front = true;
    int_x[0] = int_x_dflt[0];
    int_x[1] = int_x_dflt[1];
    int_y[0] = int_y_dflt[0];
    int_y[1] = int_y_dflt[1];
    vo[0] = vo[1] = 0;
    for( unsigned int i=0; i<show_n; ++i )
        show[i] = true;
}

void Plotter::SetViewOrigin(float x0, float y0)
{
    vo[0] = x0;
    vo[1] = y0;
}

void Plotter::SetMode(unsigned mode, bool track)
{
    plot_mode = mode;
    track_front = track;
}

void Plotter::SetLineThickness(float t)
{
    lineThickness = t;
}

void Plotter::Keyboard(View&, unsigned char key, int x, int y, bool pressed)
{
    if( pressed )
    {
        if( key == 't' ) {
            track_front = !track_front;
        }else if( key == 'c' ) {
            log->Clear();
            print_report("Plotter: Clearing data\n");
        }else if( key == 's' ) {
            log->Save("./log.csv");
            print_report("Plotter: Saving to log.csv\n");
        }else if( key == 'm' ) {
            draw_mode = (draw_mode+1)%draw_modes_n;
        }else if( key == 'p' ) {
            plot_mode = (plot_mode+1)%modes_n;
            ResetView();
            if( plot_mode==XY ) {
                int_x[0] = int_y[0];
                int_x[1] = int_y[1];
                track_front = false;
            }
        }else if( key == 'r' ) {
            print_report("Plotter: Reset viewing range\n");
            ResetView();
        }else if( key == 'a' || key == ' ' ) {
            print_report("Plotter: Auto scale\n");
            if( plot_mode==XY && log->sequences.size() >= 2)
            {
                int_x[0] = log->Sequence(0).Min();
                int_x[1] = log->Sequence(0).Max();
                int_y[0] = log->Sequence(1).Min();
                int_y[1] = log->Sequence(1).Max();
            }else{
                float min_y = numeric_limits<float>::max();
                float max_y = numeric_limits<float>::min();
                for( unsigned int i=0; i<log->sequences.size(); ++i )
                {
                    if( i>=show_n || show[i] )
                    {
                        min_y = std::min(min_y,log->Sequence(i).Min());
                        max_y = std::max(max_y,log->Sequence(i).Max());
                    }
                }
                if( min_y < max_y )
                {
                    int_y[0] = min_y;
                    int_y[1] = max_y;
                }
            }
        }else if( '1' <= key && key <= '9' ) {
            show[key-'1'] = !show[key-'1'];
        }
    }
}

void Plotter::ScreenToPlot(int x, int y)
{
    mouse_xy[0] = vo[0] + int_x[0] + (int_x[1]-int_x[0]) * (x - v.l) / (float)v.w;
    mouse_xy[1] = vo[1] + int_y[0] + (int_y[1]-int_y[0]) * (y - v.b) / (float)v.h;
}

void Plotter::Mouse(View&, MouseButton button, int x, int y, bool pressed, int button_state)
{
    last_mouse_pos[0] = x;
    last_mouse_pos[1] = y;
    mouse_state = button_state;
    
    if(button == MouseWheelUp || button == MouseWheelDown)
    {
        //const float mean = (int_y[0] + int_y[1])/2.0;
        const float scale = 1.0f + ((button == MouseWheelDown) ? 0.1 : -0.1);
        //    int_y[0] = scale*(int_y[0] - mean) + mean;
        //    int_y[1] = scale*(int_y[1] - mean) + mean;
        int_y[0] = scale*(int_y[0]) ;
        int_y[1] = scale*(int_y[1]) ;
    }
    
    ScreenToPlot(x,y);
}

void Plotter::MouseMotion(View&, int x, int y, int button_state)
{
    mouse_state = button_state;
    const int d[2] = {x-last_mouse_pos[0],y-last_mouse_pos[1]};
    const float is[2] = {int_x[1]-int_x[0],int_y[1]-int_y[0]};
    const float df[2] = {is[0]*d[0]/(float)v.w, is[1]*d[1]/(float)v.h};
    
    if( button_state == MouseButtonLeft )
    {
        track_front = false;
        //int_x[0] -= df[0];
        //int_x[1] -= df[0];
        vo[0] -= df[0];
        
        //    interval_y[0] -= df[1];
        //    interval_y[1] -= df[1];
    }else if(button_state == MouseButtonMiddle )
    {
        //int_y[0] -= df[1];
        //int_y[1] -= df[1];
        vo[1] -= df[1];
    }else if(button_state == MouseButtonRight )
    {
        const double c[2] = {
            track_front ? int_x[1] : (int_x[0] + int_x[1])/2.0,
            (int_y[0] + int_y[1])/2.0
        };
        const float scale[2] = {
            1.0f + (float)d[0] / (float)v.w,
            1.0f - (float)d[1] / (float)v.h,
        };
        int_x[0] = scale[0]*(int_x[0] - c[0]) + c[0];
        int_x[1] = scale[0]*(int_x[1] - c[0]) + c[0];
        int_y[0] = scale[1]*(int_y[0] - c[1]) + c[1];
        int_y[1] = scale[1]*(int_y[1] - c[1]) + c[1];
    }
    
    last_mouse_pos[0] = x;
    last_mouse_pos[1] = y;
}

void Plotter::Special(View&, InputSpecial inType, float x, float y, float p1, float p2, float p3, float p4, int button_state)
{
    mouse_state = button_state;

    if(inType == InputSpecialScroll) {
        const float d[2] = {p1,-p2};
        const float is[2] = {int_x[1]-int_x[0],int_y[1]-int_y[0]};
        const float df[2] = {is[0]*d[0]/(float)v.w, is[1]*d[1]/(float)v.h};

        vo[0] -= df[0];
        vo[1] -= df[1];
        if(df[0] > 0) {
            track_front = false;
        }
    } else if(inType == InputSpecialZoom) {
        const float scale = (1-p1);
        const double c[2] = {
            track_front ? int_x[1] : (int_x[0] + int_x[1])/2.0,
            (int_y[0] + int_y[1])/2.0
        };
        
        if(button_state & KeyModifierCmd) {        
            int_y[0] = scale*(int_y[0] - c[1]) + c[1];
            int_y[1] = scale*(int_y[1] - c[1]) + c[1];        
        }else{
            int_x[0] = scale*(int_x[0] - c[0]) + c[0];
            int_x[1] = scale*(int_x[1] - c[0]) + c[0];
        }
    }
}


Plotter& CreatePlotter(const string& name, DataLog* log)
{
    Plotter* v = new Plotter(log);
    context->named_managed_views[name] = v;
    context->base.views.push_back(v);
    return *v;
}

} // namespace pangolin
