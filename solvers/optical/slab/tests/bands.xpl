<plask loglevel="detail">

<defines>
  <define name="r" value="0.3"/>
  <define name="h" value="0.6"/>
  <define name="nk" value="6"/>
  <define name="N" value="2"/>
  <define name="start" value="0.001"/>
  <define name="end" value="0.500"/>
  <define name="step" value="0.001"/>
</defines>

<materials>
  <material name="mat" base="semiconductor">
    <Nr>3.5</Nr>
  </material>
</materials>

<geometry>
  <cartesian3d name="main" axes="x,y,z" back="periodic" front="periodic" left="periodic" right="periodic" bottom="air" top="air">
    <stack x="0" y="0">
      <align name="layer" xcenter="0.2" ycenter="0.1" zcenter="0">
        <cuboid material="mat" dx="{sqrt(3)}" dy="1.0" dz="{h/2}"/>
        <clip back="{-sqrt(3)/2}" front="{sqrt(3)/2}" left="-0.5" right="0.5">
          <lattice ax="0.0" ay="1.0" az="0.0" bx="{sqrt(3)/2}" by="0.5" bz="0.0">
            <segments>0 -1; -1 1; 0 1; 1 -1</segments>
            <cylinder material="air" radius="{r}" height="{h/2}"/>
          </lattice>
        </clip>
      </align>
      <zero/>
      <again ref="layer"/>
    </stack>
  </cartesian3d>
</geometry>

<solvers>
  <optical name="OPTICAL" solver="Fourier3D" lib="slab">
    <geometry ref="main"/>
    <expansion lam0="980." oversampling="8" refine="0" size="{N}"/>
    <interface position="0."/>
    <lattice long0="0.0" long1="{sqrt(3)/2}" tran0="1.0" tran1="0.5"/>
  </optical>
</solvers>

<script><![CDATA[
msh = mesh.Rectangular3D(linspace(-2., 2., 201), linspace(-2., 2., 201), [0.])
refindex = OPTICAL.outRefractiveIndex(msh)
(x0, y0), (x1, y1) = OPTICAL.lattice
plot_field(refindex, None, plane='yx', comp='x', interpolation='nearest')
plot_geometry(OPTICAL.geometry, plane='yx', color='w')
plot([0., x0], [0., y0], color='m')
plot([0., x1], [0., y1], color='m')
aspect('equal')
gcf().canvas.set_window_title('Refractive Index')

k0s = arange(2*pi*start, 2*pi*end+1e-12, 2*pi*step)

def browse():
    dets = abs(OPTICAL.get_determinant(k0=k0s))
    mins = [k0s[i] for i in range(1, len(dets)-1) if dets[i-1] >= dets[i] and dets[i] < dets[i+1]]
    print_log('data', 'Approx. bands:', ', '.join('{:.2f}'.format(x) for x in mins))
    return mins


# Triangular lattice: K = [4*pi/(3),0] M = [pi, pi/(sqrt(3))]
sqr3 = sqrt(3)
sqr32 = sqr3 / 2.

# Gamma -> M
n = int(sqr3/2 * nk)
wavevectors = [ (pi * k/n, pi/sqr3 * k/n) for k in range(n) ]
graphpos = [ pi * sqr32 * k/n for k in range(n) ]

# M -> K
n = int(nk/2.)
wavevectors.extend( [ (pi * ( 1 + 1./3.*k/n), pi/sqr3 * (n-k)/n) for k in range(n) ] )
graphpos.extend( [ pi * (sqr32 + 0.5 * k/n) for k in range(n) ] )

# K -> Gamma
n = nk
wavevectors.extend( [ (4*pi/3 * (n-k)/nk, 0.) for k in range(n+1) ] )
graphpos.extend( [ pi * (sqr32 + 0.5 + 1. * k/n) for k in range(n+1) ] )



## Do the computations ##

results = []

with open('bands.out', 'w') as out:
    out.write("#_po__ _kx__ _ky__  omega/c\n")

    for i,(K,pos) in enumerate(zip(wavevectors, graphpos)):
        OPTICAL.ktran, OPTICAL.klong = K

        modes = set(OPTICAL.find_mode(k0=k0) for k0 in browse())

        # test for spurious modes (not optimal method, but should work)
        bands = [k0 for k0 in (OPTICAL.modes[m].k0.real for m in modes) if k0 >= 0]

        for k0 in bands:
            results.append((pos, K[0], K[1], k0))
            out.write("{:6.3f} {:5.3f} {:5.3f}  {:.4f}\n".format(pos, K[0], K[1], k0))
        out.flush()

        print_log('result', K, ':', ' '.join(str(k0) for k0 in bands), "   [{:.1f}%]".format(100.*(i+1)/len(graphpos)))

results = array(results).T

figure()
try:
    reference = loadtxt('bands.dat', unpack=True)
except IOError:
    pass
else:
    plot(reference[0], reference[3], '.', color='0.7')

plot(results[0], results[3], '.', color='maroon')

xticks([0., pi*sqr32, pi*(sqr32+0.5), pi*(sqr32+1.5)], ['$\\Gamma$', 'M', 'K', '$\\Gamma$'])
xlim(0., pi*(sqr32+1.5))
grid(axis='x')

ylabel("$\\omega/c$")

tight_layout(0.1)
gcf().canvas.set_window_title('Photonic Bands')
savefig('bands.png')
show()
]]></script>

</plask>
