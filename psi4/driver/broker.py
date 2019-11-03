#
# @BEGIN LICENSE
#
# Psi4: an open-source quantum chemistry software package
#
# Copyright (c) 2019 The Psi4 Developers.
#
# The copyrights for code used from other parties are included in
# the corresponding files.
#
# This file is part of Psi4.
#
# Psi4 is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, version 3.
#
# Psi4 is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with Psi4; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# @END LICENSE
#

import string
import sys
import time
import numpy as np
from numpy.testing import assert_array_almost_equal as assert_equal

import psi4
try:
    from ipi.interfaces.sockets import Client
    ipi_available = True
except ImportError:
    ipi_available = False

    # Define Client to enable testing of the Broker in the unittests
    class Client():
        pass


class IPIBroker(Client):
    def __init__(self, options, serverdata=False, molecule=None):
        self.serverdata = serverdata
        if not ipi_available:
            psi4.core.print_out("i-pi is not available for import: ")
            psi4.core.print_out("The broker infrastructure will not be available!\n")
            super(IPIBroker, self).__init__()
        elif serverdata:
            mode, address, port = serverdata.split(":")
            mode = string.lower(mode)
            super(IPIBroker, self).__init__(address=address, port=port, mode=mode)
        else:
            super(IPIBroker, self).__init__(_socket=False)
        self.options = options

        if molecule is None:
            molecule = psi4.core.get_active_molecule()
        self.initial_molecule = molecule
        assert self.initial_molecule.orientation_fixed() == True, "Orientation must be fixed!"
        assert self.initial_molecule.point_group().symbol() == "c1", "Symmetry must be 'c1'!"

        names = [self.initial_molecule.symbol(i) for i in range(self.initial_molecule.natom())]
        psi4.core.print_out("Initial atoms %s\n" % names)
        self.atoms_list = names

        psi4.core.print_out("PSI4 options:\n")
        for item, value in self.options.items():
            psi4.core.print_out("%s %s\n" % (item, value))
            if item not in ["LOT", "multiplicity", "charge"]:
                psi4.core.set_global_option(item, value)
        psi4.core.IO.set_default_namespace("xwrapper")

        self.timing = {}

        atoms = np.array(self.initial_molecule.geometry())
        psi4.core.print_out("Initial atoms %s\n" % atoms)
        psi4.core.print_out("Force:\n")

    def calculate_force(self, pos=None, **kwargs):
        """Fetch force, energy of PSI.

        Arguments:
        - pos: positions of the atoms as array. If None, the positions of the current active
          molecule is used.
        """
        if pos is None:
            molecule = psi4.core.get_active_molecule()
            pos = np.array(molecule.geometry())

        self.frc, self.pot = self.callback(pos, **kwargs)
        return self.frc, self.pot

    def callback(self, pos, **kwargs):
        """Initialize psi with new positions and calculate force.

        Arguments:
        - pos: positions of the atoms as array.
        """

        self.initial_molecule.set_geometry(psi4.core.Matrix.from_array(pos))

        self.calculate_gradient(self.options["LOT"], **kwargs)

        self.pot = psi4.variable('CURRENT ENERGY')
        self.frc = -np.array(self.grd)

        return self.frc, np.float64(self.pot)

    def calculate_gradient(self, LOT, bypass_scf=False, **kwargs):
        """Calculate the gradient with @LOT.

        When bypass_scf=True a hf energy calculation has been done before.
        """
        start = time.time()
        self.grd = psi4.gradient(LOT, bypass_scf=bypass_scf, **kwargs)
        time_needed = time.time() - start
        self.timing[LOT] = self.timing.get(LOT, []) + [time_needed]


def ipi_broker(molecule=None, serverdata=False, options=None):
    """ Run IPIBroker to connect to i-pi

    Arguments:
        molecule: Initial molecule
        serverdata: Configuration where to connect to ipi
        options: LOT, multiplicity and charge
    """
    b = IPIBroker(molecule=molecule, serverdata=serverdata, options=options)

    try:
        if b.serverdata:
            b.run()
        else:
            return b

    except KeyboardInterrupt:
        psi4.core.print_out("Killing IPIBroker\n")
        b.__del__()
        sys.exit(1)
