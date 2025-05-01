import pytest
from unittest.mock import Mock, patch
from Configuration import IF2COSMOS  # Replace with your actual class and module names

# Sample configuration and response to be used as return values in mocks
sample_config = {
    'tm': {
        'packet1': {
            'param1': {'type': 'block'},
            'param2': {'type': 'value'}
        }
    }
}

sample_response = {
    'result': 'data',
    'result': {'raw': 'raw_data'}
}

# Test case for receiveTelemetry function
def test_receiveTelemetry():
    # Create an instance of the class containing the receiveTelemetry method
    instance = IF2COSMOS()
    instance.subsystem = 'Subsystem1'
    instance.config = sample_config
    instance.payload = {}

    # Mock the io.prnt and __getResponse methods
    instance.io = Mock()
    instance.__getResponse = Mock()

    # Set the return value for the __getResponse mock
    instance.__getResponse.return_value = sample_response

    # Use patch to mock the sys.exit call
    with patch('sys.exit') as mock_exit:
        response = instance.receiveTelemetry('packet1', ['param1', 'param2'])

        # Assertions to check if the function behaves as expected
        assert response == {'param1': 'raw_data', 'param2': 'data'}
        instance.io.prnt.assert_called_with('Received TM for param2 of packet1 of Subsystem1', P2S)
        mock_exit.assert_not_called()

# Run the test
if __name__ == "__main__":
    pytest.main()


