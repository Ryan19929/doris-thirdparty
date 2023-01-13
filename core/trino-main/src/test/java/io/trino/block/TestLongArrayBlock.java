/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package io.trino.block;

import io.airlift.slice.Slice;
import io.trino.spi.block.BlockBuilder;
import io.trino.spi.block.LongArrayBlock;
import io.trino.spi.block.LongArrayBlockBuilder;
import io.trino.spi.block.VariableWidthBlockBuilder;
import org.testng.annotations.Test;

import java.util.Optional;

import static io.airlift.slice.SizeOf.SIZE_OF_LONG;
import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertTrue;

public class TestLongArrayBlock
        extends AbstractTestBlock
{
    @Test
    public void test()
    {
        Slice[] expectedValues = createTestValue(17);
        assertFixedWithValues(expectedValues);
        assertFixedWithValues(alternatingNullValues(expectedValues));
    }

    @Test
    public void testCopyPositions()
    {
        Slice[] expectedValues = alternatingNullValues(createTestValue(17));
        BlockBuilder blockBuilder = createBlockBuilderWithValues(expectedValues);
        assertBlockFilteredPositions(expectedValues, blockBuilder.build(), 0, 2, 4, 6, 7, 9, 10, 16);
    }

    @Test
    public void testLazyBlockBuilderInitialization()
    {
        Slice[] expectedValues = createTestValue(100);
        BlockBuilder emptyBlockBuilder = new VariableWidthBlockBuilder(null, 0, 0);

        BlockBuilder blockBuilder = new VariableWidthBlockBuilder(null, expectedValues.length, 32 * expectedValues.length);
        assertEquals(blockBuilder.getSizeInBytes(), emptyBlockBuilder.getSizeInBytes());
        assertEquals(blockBuilder.getRetainedSizeInBytes(), emptyBlockBuilder.getRetainedSizeInBytes());

        writeValues(expectedValues, blockBuilder);
        assertTrue(blockBuilder.getSizeInBytes() > emptyBlockBuilder.getSizeInBytes());
        assertTrue(blockBuilder.getRetainedSizeInBytes() > emptyBlockBuilder.getRetainedSizeInBytes());

        blockBuilder = blockBuilder.newBlockBuilderLike(null);
        assertEquals(blockBuilder.getSizeInBytes(), emptyBlockBuilder.getSizeInBytes());
        assertEquals(blockBuilder.getRetainedSizeInBytes(), emptyBlockBuilder.getRetainedSizeInBytes());
    }

    @Test
    public void testEstimatedDataSizeForStats()
    {
        Slice[] expectedValues = createTestValue(100);
        assertEstimatedDataSizeForStats(createBlockBuilderWithValues(expectedValues), expectedValues);
    }

    @Test
    public void testCompactBlock()
    {
        long[] longArray = {0L, 0L, 1L, 2L, 3L, 4L};
        boolean[] valueIsNull = {false, true, false, false, false, false};

        testCompactBlock(new LongArrayBlock(0, Optional.empty(), new long[0]));
        testCompactBlock(new LongArrayBlock(longArray.length, Optional.of(valueIsNull), longArray));
        testIncompactBlock(new LongArrayBlock(longArray.length - 1, Optional.of(valueIsNull), longArray));
    }

    private void assertFixedWithValues(Slice[] expectedValues)
    {
        BlockBuilder blockBuilder = createBlockBuilderWithValues(expectedValues);
        assertBlock(blockBuilder, expectedValues);
        assertBlock(blockBuilder.build(), expectedValues);
    }

    private static BlockBuilder createBlockBuilderWithValues(Slice[] expectedValues)
    {
        LongArrayBlockBuilder blockBuilder = new LongArrayBlockBuilder(null, expectedValues.length);
        writeValues(expectedValues, blockBuilder);
        return blockBuilder;
    }

    private static void writeValues(Slice[] expectedValues, BlockBuilder blockBuilder)
    {
        for (Slice expectedValue : expectedValues) {
            if (expectedValue == null) {
                blockBuilder.appendNull();
            }
            else {
                blockBuilder.writeLong(expectedValue.getLong(0)).closeEntry();
            }
        }
    }

    private static Slice[] createTestValue(int positionCount)
    {
        Slice[] expectedValues = new Slice[positionCount];
        for (int position = 0; position < positionCount; position++) {
            expectedValues[position] = createExpectedValue(SIZE_OF_LONG);
        }
        return expectedValues;
    }

    @Override
    protected boolean isByteAccessSupported()
    {
        return false;
    }

    @Override
    protected boolean isShortAccessSupported()
    {
        return false;
    }

    @Override
    protected boolean isIntAccessSupported()
    {
        return false;
    }

    @Override
    protected boolean isSliceAccessSupported()
    {
        return false;
    }
}