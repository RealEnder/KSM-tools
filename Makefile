# by Alex Stanev <alex@stanev.org>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

CC	= gcc
CFLAGS += -Wall -Wextra -Wcast-align -Wpointer-arith -Wcast-align -Wno-sign-compare -Wconversion

.PHONY: all clean dist

all:    madv_ksm.so

dist:
	rm -f *.o

clean:  dist
	rm -f madv_ksm.so
	find -name '*~' -exec rm {} \;

madv_ksm.so: madv_ksm.c Makefile
	$(CC) $(CFLAGS) -shared -fPIC -O3 $< -o $@ -ldl
